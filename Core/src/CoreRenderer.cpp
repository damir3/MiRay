#include "CoreRenderer.h"
#include "CoreInstance.h"
#include "RenderUtils.h"
#include "Gizmo.h"
#include "Materials/MaterialImpl.h"
#include "Renderers/DummyRenderer.h"
#include <OpenImageDenoise/oidn.hpp>
#include <QDebug>
#include <QEvent>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QWheelEvent>
#include "../../Shared/Interfaces/Settings.h"

constexpr int DUMMY_MAX_ITERATIONS = 4;

#define CURRENT_TIME	std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count()

// ------------------------------------------------------------------------ //

KeyboardGlobalFilter::KeyboardGlobalFilter(Gizmo * gizmo)
	: m_gizmo(*gizmo)
{
	assert(gizmo); // instantiate me after gizmo
	qApp->installEventFilter(this);
}

bool KeyboardGlobalFilter::eventFilter(QObject*, QEvent* event)
{
	const auto type = event->type();
	if (QEvent::KeyPress != type && QEvent::KeyRelease != type)
		return false;

	auto keyEvent = static_cast<QKeyEvent*>(event);
	const auto altPressed = (keyEvent->modifiers() & Qt::AltModifier) != 0;
	if (m_altPressed == altPressed)
		return false;

	m_altPressed = altPressed;
	m_gizmo.setLocalMode(type == QEvent::KeyPress);

	return false;
}

// ------------------------------------------------------------------------ //

CoreRenderer::CoreRenderer(CoreInstance &core, float devicePixelRatio, bool offscreen)
	: m_core(core)
	, m_scene(core.scene())
	, m_dummyRenderer(std::make_unique<DummyRenderer>(core.scene()))
	, m_gizmo(new Gizmo(*this, core))
	, m_altGrabber(m_gizmo.get())
	, m_dummyRendererStartTime(CURRENT_TIME)
	, m_pixelRatio(devicePixelRatio)
{
	core.initOIDN();
	m_oidnFilter = core.oidnDevice() ? core.oidnDevice().newFilter("RT") : oidn::FilterRef();

	connect(&m_scene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
	connect(&m_scene, SIGNAL(nodeChanged(const INode *, eNodeChanged)), this, SLOT(onNodeChanged(const INode *, eNodeChanged)));
	connect(&m_scene, SIGNAL(cameraChanged()), this, SLOT(onCameraChanged()));
	connect(&m_scene, SIGNAL(sceneLock(eSceneInternalModification)), this, SLOT(onSceneLock(eSceneInternalModification)));
	connect(&m_scene, SIGNAL(sceneUnlock(eSceneInternalModification)), this, SLOT(onSceneUnlock(eSceneInternalModification)));
	connect(&m_scene.materialManager(), SIGNAL(materialChanged(const IMaterial *, bool)), this, SLOT(onMaterialChanged()));

	m_renderThread.reset(new std::thread(&CoreRenderer::threadFunc, this)); // start render thread
}

CoreRenderer::~CoreRenderer()
{
	qDebug() << "delete CoreRenderer";
}

void CoreRenderer::invalidate()
{
	qDebug() << "CoreRenderer::invalidate()";
	disconnect(&m_scene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
	disconnect(&m_scene, SIGNAL(nodeChanged(const INode *, eNodeChanged)), this, SLOT(onNodeChanged(const INode *, eNodeChanged)));
	disconnect(&m_scene, SIGNAL(cameraChanged()), this, SLOT(onCameraChanged()));
	disconnect(&m_scene, SIGNAL(sceneLock(eSceneInternalModification)), this, SLOT(onSceneLock(eSceneInternalModification)));
	disconnect(&m_scene, SIGNAL(sceneUnlock(eSceneInternalModification)), this, SLOT(onSceneUnlock(eSceneInternalModification)));
	disconnect(&m_scene.materialManager(), SIGNAL(materialChanged(const IMaterial *, bool)), this, SLOT(onMaterialChanged()));

	m_stopThread = true;
	m_core.renderer()->restart(true);
	m_dummyRenderer->restart(true);
	if (m_renderThread)
		m_renderThread->join();
}

QOpenGLFramebufferObject *CoreRenderer::createFramebufferObject(const QSize &size)
{
	qDebug() << "CoreRenderer::createFramebufferObject" << size.width() << size.height();
	if (!m_stopThread) {
		initializeGL();
		resizeGL(size.width(), size.height());
	}

	QOpenGLFramebufferObjectFormat format;
	format.setAttachment(QOpenGLFramebufferObject::Depth);
	return new QOpenGLFramebufferObject(size, format);
}

void CoreRenderer::synchronize(QQuickFramebufferObject *item)
{
	m_visible = item->isVisible();
	m_quickWindow = item->window();
}

void CoreRenderer::initializeGL()
{
	if (m_gl)
		return;

	auto *context = QOpenGLContext::currentContext();
	connect(context, &QOpenGLContext::aboutToBeDestroyed, this, &CoreRenderer::destroyGL);

	m_gl.reset(new DrawGL(context));

	auto glExtensions = (char *)m_gl->glGetString(GL_EXTENSIONS);
	LogInformation() << QString().asprintf("OpenGL extensions: %s", glExtensions);
	m_supportTextureNonPowerOfTwo = strstr(glExtensions, "GL_ARB_texture_non_power_of_two") != nullptr;
	m_supportTextureFloat = strstr(glExtensions, "GL_ARB_texture_float") != nullptr || strstr(glExtensions, "GL_OES_texture_float") != nullptr;

	m_gl->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);

	const uint32_t checkers_data[] = { 0xFFFFFFFF, 0xFFA0A0A0, 0xFFA0A0A0, 0xFFFFFFFF };
	m_bgTexture = createTexture2D(m_gl.get(), 2, 2, checkers_data, GL_REPEAT, GL_NEAREST);

	m_gizmo->initializeGL(m_gl.get(), m_pixelRatio);

	assert(GL_NO_ERROR == m_gl->glGetError());
}

void CoreRenderer::destroyGL()
{
	if (!m_gl)
		return;

	auto *context = QOpenGLContext::currentContext();
	if (context) {
		disconnect(context, &QOpenGLContext::aboutToBeDestroyed, this, &CoreRenderer::destroyGL);
	}

	assert(GL_NO_ERROR == m_gl->glGetError());

	DELETE_TEXTURE(m_rmTexture);
	DELETE_TEXTURE(m_bgTexture);

	m_gizmo->destroyGL();

	m_gl.reset();
}

void CoreRenderer::resizeGL(int width, int height)
{
	m_mutexSize.lock();
	m_size = QSize((int)(width / m_pixelRatio), (int)(height / m_pixelRatio));
	m_invSize = vec2(1.f / (float)m_size.width(), 1.f / (float)m_size.height());
	m_screenAspect = (float)width / (float)height;
	m_mutexSize.unlock();

	m_gl->setViewportSize(width, height);

	onCameraChanged();

	assert(GL_NO_ERROR == m_gl->glGetError());
}

void CoreRenderer::render()
{
	assert(GL_NO_ERROR == m_gl->glGetError());
	m_gl->glClearColor(1.f, 1.f, 1.f, 0.f);
	m_gl->glClearDepthf(1.f);
	m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLenum err = m_gl->glGetError();
	if (err != GL_NO_ERROR || m_stopThread)
		return;

	m_gl->glEnable(GL_CULL_FACE);
	m_gl->glFrontFace(GL_CW);
	m_gl->glDepthFunc(GL_LEQUAL);
	m_gl->glDisable(GL_DEPTH_TEST);
	m_gl->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	updateRenderMap();

	drawTransparentBackground();

	if (m_rmTexture)
		m_gl->drawTexture(mat4(1.f), m_rmTexture, WHITE_COLOR, m_rmTexCoords, true);

	m_gizmo->draw();

	drawAspectBlinds() ;

	if (m_quickWindow) {
		m_quickWindow->resetOpenGLState();
	}

	assert(GL_NO_ERROR == m_gl->glGetError());
}

void CoreRenderer::drawTransparentBackground() const
{
	m_gl->glDisable(GL_BLEND);
	m_gl->drawTexture(mat4(1.f), m_bgTexture, WHITE_COLOR, vec2(m_size.width() / 64.f, m_size.height() / 64.f));
	m_gl->glEnable(GL_BLEND);
}

void CoreRenderer::drawAspectBlinds() const
{
	m_gl->glEnable(GL_BLEND);
	m_gl->glDisable(GL_DEPTH_TEST);

	const vec4 color(0.f, 0.f, 0.f, 0.5f);
	const auto cameraAspect = m_scene.camera().aspect().get();
	const auto screenAspect = (float)m_size.width() / (float)m_size.height();
	if (screenAspect < cameraAspect) {
		const auto y = screenAspect / cameraAspect;
		m_gl->drawRect(vec2(-1.f, y), vec2(1.f, 1), color);
		m_gl->drawRect(vec2(-1.f, -1.f), vec2(1.f, -y), color);
	} else {
		const auto x = cameraAspect / screenAspect;
		m_gl->drawRect(vec2(-1.f, -1.f), vec2(-x, 1.f), color);
		m_gl->drawRect(vec2(x, -1.f), vec2(1.f, 1.f), color);
	}
}

ImagePtr CoreRenderer::createScreenshot(int width, int height)
{
	std::lock_guard<std::mutex> lock(m_mutexRenderMap);
	return m_core.appContext().imageManager()->scaleImage(m_renderMap, width, height, eImageFormat::RGBA, eImageDataType::Float, eScaleFilter::Triangle);
}

GLsizei CoreRenderer::getTextureSize(GLsizei size) const
{
	if (m_supportTextureNonPowerOfTwo)
		return std::min<GLsizei>(size, m_maxTextureSize);

	GLsizei texSize = 1;
	while (texSize < size)
		texSize <<= 1;
	return std::min<GLsizei>(texSize, m_maxTextureSize);
}

void CoreRenderer::updateTexture(GLuint & texture, QSize & texSize, vec2 & texCoords, const IImage * image, const RectI & rc)
{
	assert(image->format() == eImageFormat::RGBA);
	assert(image->dataType() == eImageDataType::Float);
	const auto texWidth = getTextureSize(image->width());
	const auto texHeight = getTextureSize(image->height());
	if (!texture || texWidth != texSize.width() || texHeight != texSize.height()) {
		assert(GL_NO_ERROR == m_gl->glGetError());
		DELETE_TEXTURE(texture);

		texSize = QSize(texWidth, texHeight);

		m_gl->glGenTextures(1, &texture);
		m_gl->glBindTexture(GL_TEXTURE_2D, texture);
		m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		assert(GL_NO_ERROR == m_gl->glGetError());

		if (m_supportTextureFloat)
			m_gl->glTexImage2D(GL_TEXTURE_2D, 0, QOpenGLContext::currentContext()->isOpenGLES() ? GL_RGBA : GL_RGBA32F, texWidth, texHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		else
			m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		assert(GL_NO_ERROR == m_gl->glGetError());
	}

	auto convertData = [](uint8_t *dest, const float *src, int count) {
		while (count-- > 0) {
			*dest++ = F2B(std::min(*src++, 1.f));
			*dest++ = F2B(std::min(*src++, 1.f));
			*dest++ = F2B(std::min(*src++, 1.f));
			*dest++ = F2B(*src++);
		}
	};

	m_gl->glBindTexture(GL_TEXTURE_2D, texture);
	const auto * imageData = static_cast<const float *>(image->rawData());
	if (image->width() <= texWidth && rc.bottom <= texHeight) {
		const GLsizei width = rc.right;
		const GLsizei height = rc.bottom;
		if (m_supportTextureFloat)
			m_gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width(), height, GL_RGBA, GL_FLOAT, imageData);
		else
		{
			static std::vector<uint8_t> rowDataU8;
			if (rowDataU8.size() < (size_t)texWidth * 4)
				rowDataU8.resize(texWidth * 4);
			for (GLsizei y = 0; y < height; y++) {
				convertData(rowDataU8.data(), imageData + y * image->width() * 4, width);
				m_gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, rowDataU8.data());
			}
		}

		texCoords.x = width / (float)texWidth;
		texCoords.y = height / (float)texHeight;
	} else {
		const GLsizei width = std::min<int>(texWidth, rc.right);
		const GLsizei height = std::min<int>(texHeight, rc.bottom);
		auto dy = float(rc.bottom - 1) / float(height - 1);
		auto fy = 0.5f / float(height) - 0.5f / float(rc.bottom);

		static std::vector<float> rowDataF32;
		if (rowDataF32.size() < (size_t)width * 4)
			rowDataF32.resize(width * 4);

		for (GLsizei y = 0; y < height; y++, fy += dy) {
			fillTextureLineRGBA(rowDataF32.data(), width, fy, rc.right, rc.bottom, imageData, image->width() * 4);
			if (m_supportTextureFloat)
				m_gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, 1, GL_RGBA, GL_FLOAT, rowDataF32.data());
			else
			{
				static std::vector<uint8_t> rowDataU8;
				if (rowDataU8.size() < (size_t)width * 4)
					rowDataU8.resize(width* 4);
				convertData(rowDataU8.data(), rowDataF32.data(), width);
				m_gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, rowDataU8.data());
			}
		}

		texCoords.x = width / (float)texWidth;
		texCoords.y = height / (float)texHeight;
	}
	assert(GL_NO_ERROR == m_gl->glGetError());
}

void CoreRenderer::updateRenderMap()
{
	if (m_rmUpdateTexture) {
		std::lock_guard<std::mutex> lock(m_mutexRenderMap);
		updateTexture(m_rmTexture, m_rmTexSize, m_rmTexCoords, m_renderMap.get(), m_renderMapRect);
		m_rmUpdateTexture = false;
	}
}

float CoreRenderer::getX(int x) const
{
	return (((float)x * m_invSize.x) * 2.f - 1.f) * m_aspectScale.x;
}

float CoreRenderer::getY(int y) const
{
	return (((float)y * m_invSize.y) * -2.f + 1.f) * m_aspectScale.y;
}

vec3 CoreRenderer::getFrustumPosition(int x, int y, float z) const
{
	return m_scene.getFrustumPosition(getX(x), getY(y), z);
}

void CoreRenderer::setShowNormal(bool b)
{
	m_showNormal = b;
	m_gizmo->updateNormal();
	update();
}

void CoreRenderer::setMaxIterations(int n)
{
	m_maxIterations = std::clamp<int>(n, PREVIEW_FRAMES_MIN, PREVIEW_FRAMES_MAX);
}

IGeometry * CoreRenderer::getGeometry(int x, int y) const
{
	if (!m_scene.isValid())
		return nullptr;

	Ray ray;
	ray.init(getFrustumPosition(x, y, -1.f), getFrustumPosition(x, y, 1.f));
	rtcIntersect1(m_scene.rtcScene(), &ray);

	if (ray.hit.primID != RTC_INVALID_GEOMETRY_ID && ray.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		if (auto node = MeshNode::getByRay(m_scene.rtcScene(), ray))
			return node->getGeometryByRay(ray);
	}

	return nullptr;
}

void CoreRenderer::onSelectionChanged()
{
	m_gizmo->onSelectionChanged();
}

void CoreRenderer::onNodeChanged(const INode*, eNodeChanged)
{
}

void CoreRenderer::onCameraChanged()
{
	auto & camera = m_scene.camera();
	auto screenAspect = (float)m_size.width() / m_size.height();
	auto cameraAspect = camera.aspect().get();
	m_aspectScale = screenAspect < cameraAspect ? vec2(1.f, cameraAspect / screenAspect) : vec2(screenAspect / cameraAspect, 1.f);
	m_matView = camera.viewMatrix();
	m_matProj = camera.projMatrix();
	m_matProj[0][0] /= m_aspectScale.x;
	m_matProj[1][1] /= m_aspectScale.y;
	m_matViewProj = m_matProj * m_matView;
	if (m_gl)
		m_gl->setCameraInfo(m_matView, m_matProj, camera.nearZ().get());
	m_gizmo->onCameraChanged();

	m_core.renderer()->restart(true);
	m_dummyRenderer->restart(true);
}

void CoreRenderer::onMaterialChanged()
{
	m_core.renderer()->restart(true);
	m_dummyRenderer->restart(true);
}

void CoreRenderer::onLoadingDone()
{
	m_core.renderer()->restart(true);
	m_dummyRenderer->restart(true);
}

void CoreRenderer::enablePreview(bool b)
{
	m_enablePreview = b;
}

void CoreRenderer::switchToDummyRenderer()
{
	m_useDummyRenderer = true;
	m_dummyRendererStartTime = CURRENT_TIME;
}

bool CoreRenderer::getScreenPos(vec2 & out, const vec3 & pos) const
{
	auto pos4 = m_matViewProj * vec4(pos, 1.f);
	if (pos4.w <= 0.f)
		return false;

	out = reinterpret_cast<vec2 &>(pos4) / pos4.w;
	return true;
}

float CoreRenderer::getScale(const vec3 &pos) const
{
	auto center = m_matViewProj * vec4(pos, 1.f);
	return center.w / (m_size.width() * m_matProj[0][0]);
}

eGizmo CoreRenderer::getGizmo() const
{
	return m_gizmo->type();
}

void CoreRenderer::setGizmo(eGizmo gizmo)
{
	m_gizmo->setType(gizmo);
}

void CoreRenderer::hoverMoveEvent(QHoverEvent *event)
{
	if (!m_stopThread)
		m_gizmo->mouseMoveEvent(new QMouseEvent(QEvent::MouseMove, event->pos(), Qt::NoButton, Qt::NoButton, event->modifiers()));
}

void CoreRenderer::mousePressEvent(QMouseEvent *event)
{
	if (!m_stopThread)
		m_gizmo->mousePressEvent(event);
}

void CoreRenderer::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_stopThread)
		m_gizmo->mouseMoveEvent(event);
}

void CoreRenderer::mouseReleaseEvent(QMouseEvent *event)
{
	if (!m_stopThread)
		m_gizmo->mouseReleaseEvent(event);
}

void CoreRenderer::wheelEvent(QWheelEvent *event)
{
	if (!m_stopThread) {
		m_useDummyRenderer = true;
		m_dummyRendererStartTime = CURRENT_TIME;
		m_scene.camera().zoom(event->angleDelta().y() / 120.f);
	}
}

void CoreRenderer::onSceneLock(eSceneInternalModification sm)
{
	if (sm == SceneInternalModification_Animation) return;

	m_pauseRendering = true;
	m_core.renderer()->restart(true);
	m_dummyRenderer->restart(sm != SceneInternalModification_Transformation && sm != SceneInternalModification_Camera);
}

void CoreRenderer::onSceneUnlock(eSceneInternalModification sm)
{
	if (sm == SceneInternalModification_Animation) return;

	if (sm == SceneInternalModification_Geometries ||
		sm == SceneInternalModification_Pivot ||
		sm == SceneInternalModification_Transformation) {
		onSelectionChanged();
	}

	m_core.renderer()->restart(true);
	m_dummyRenderer->restart(false);
	m_pauseRendering = false;
}

void CoreRenderer::updateFrame()
{
	m_core.renderer()->restart(true);
	m_dummyRenderer->restart(true);
	update();
}

QOpenGLFramebufferObject * CoreRenderer::getFramebufferObject() const
{
	return framebufferObject();
}

void CoreRenderer::threadFunc()
{
	while (!m_stopThread) {
		if (!m_visible || !m_enablePreview) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		if (m_pauseRendering) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		m_mutexSize.lock();
		const int width = m_size.width(), height = m_size.height();
		m_mutexSize.unlock();

		if (m_core.renderer()->frameCount() > m_maxIterations || width <= 0 || height <= 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		if (!m_colorBuffer || m_colorBuffer->width() != width || m_colorBuffer->height() != height) {
			m_colorBuffer = m_core.appContext().imageManager()->createImage(width, height, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Linear);
			m_albedoBuffer = m_core.appContext().imageManager()->createImage(width, height, eImageFormat::RGB, eImageDataType::Float, eImageColorSpace::Linear);
			m_normalBuffer = m_core.appContext().imageManager()->createImage(width, height, eImageFormat::RGB, eImageDataType::Float, eImageColorSpace::Linear);
		}

		bool shrinkFrame = false;
		RectI rc;
		if (m_useDummyRenderer) {
			if (m_dummyRenderer->frameCount() < DUMMY_MAX_ITERATIONS) {
				rc = m_dummyRenderer->renderFrame(*m_colorBuffer, nullptr, nullptr);
			} else if (m_gizmo->isMouseMoved() || CURRENT_TIME - m_dummyRendererStartTime < 0.5) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			} else {
				rc = m_core.renderer()->renderFrame(*m_colorBuffer, m_albedoBuffer.get(), m_normalBuffer.get());
				if (m_core.renderer()->frameCount() == 0) continue; // don't update the current frame
				shrinkFrame = true;
				m_useDummyRenderer = false;
			}
		} else {
			rc = m_core.renderer()->renderFrame(*m_colorBuffer, m_albedoBuffer.get(), m_normalBuffer.get());
			shrinkFrame = m_core.renderer()->frameCount() < DUMMY_MAX_ITERATIONS;
		}

		if (rc.width() > 0 && rc.height() > 0) {
			std::lock_guard<std::mutex> lock(m_mutexRenderMap);
			if (!m_renderMap || m_renderMap->width() != width || m_renderMap->height() != height) {
				m_renderMap = m_core.appContext().imageManager()->createImage(width, height, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Linear);
				if (m_oidnFilter) {
					m_oidnFilter.setImage("color", (void *)m_colorBuffer->rawData(), oidn::Format::Float3, width, height, 0, 16, 16 * width);
					m_oidnFilter.setImage("albedo", (void *)m_albedoBuffer->rawData(), oidn::Format::Float3, width, height, 0, 12, 12 * width);
					m_oidnFilter.setImage("normal", (void *)m_normalBuffer->rawData(), oidn::Format::Float3, width, height, 0, 12, 12 * width);
					m_oidnFilter.setImage("output", (void *)m_renderMap->rawData(), oidn::Format::Float3, width, height, 0, 16, 16 * width);
					m_oidnFilter.set("hdr", true);
					m_oidnFilter.commit();
				}
			}

			if (shrinkFrame) {
				rc.right /= 2;
				rc.bottom /= 2;
				for (int y = 0; y < rc.bottom;  y++) {
					auto src1 = (const float *)m_colorBuffer->rawData() + y * width * 8;
					auto src2 = src1 + width * 4;
					auto dest = (float *)m_renderMap->rawData() + y * width * 4;
					for (int x = 0; x < rc.right; x++, src1 += 8, src2 += 8) {
						*dest++ = (src1[0] + src1[4] + src2[0] + src2[4]) * 0.25f;
						*dest++ = (src1[1] + src1[5] + src2[1] + src2[5]) * 0.25f;
						*dest++ = (src1[2] + src1[6] + src2[2] + src2[6]) * 0.25f;
						*dest++ = (src1[3] + src1[7] + src2[3] + src2[7]) * 0.25f;
					}
				}
			} else {
				if (m_core.renderer()->frameCount() > m_maxIterations && m_oidnFilter && m_enableDenoise)
					m_oidnFilter.execute();
				else
					memcpy((void *)m_renderMap->rawData(), m_colorBuffer->rawData(), width * rc.bottom * 16);
			}

			m_renderMapRect = rc;
			m_rmUpdateTexture = true;
			update(); // Schedule redraw directly
		}
	}
}
