#pragma once

#include "../../Shared/Interfaces/CoreRenderer.h"
#include "../../Shared/Interfaces/Settings.h"
#include "RenderUtils.h"
#include <OpenImageDenoise/oidn.hpp>
#include <QPointer>

class DummyRenderer;
class Gizmo;
class DrawGL;
class QQuickWindow;

class KeyboardGlobalFilter : public QObject
{
	Q_OBJECT

	Gizmo &	m_gizmo;
	bool	m_altPressed = false;

	bool eventFilter(QObject * object, QEvent * event);

public:
	KeyboardGlobalFilter(Gizmo * gizmo);
};

class CoreRenderer final : public QObject, public ICoreRenderer
{
	Q_OBJECT
	Q_INTERFACES(ICoreRenderer)

	CoreInstance &		m_core;
	Scene &				m_scene;
	std::unique_ptr<DummyRenderer> m_dummyRenderer;
	bool				m_showNormal = true;
	bool 				m_enableDenoise = PREVIEW_DENOISE_DEFAULT;
	int 				m_maxIterations = PREVIEW_FRAMES_DEFAULT;
	std::unique_ptr<DrawGL> m_gl;
	std::unique_ptr<Gizmo>	m_gizmo;
	oidn::FilterRef			m_oidnFilter;

	KeyboardGlobalFilter m_altGrabber;

	std::unique_ptr<std::thread> m_renderThread;
	std::atomic<bool>	m_stopThread = false;
	std::atomic<bool>	m_enablePreview = true;
	std::atomic<bool>	m_pauseRendering = false;
	std::atomic<bool>	m_useDummyRenderer = true;
	std::atomic<double>	m_dummyRendererStartTime = 0.0;
	std::atomic<bool>	m_visible = true;
	QPointer<QQuickWindow> m_quickWindow;

	QSize		m_size = { 0, 0 };
	vec2		m_invSize = vec2(0.f);
	float		m_screenAspect = 1.f;
	vec2		m_aspectScale = vec2(1.f);
	mat4		m_matView = mat4(1.f);
	mat4		m_matProj = mat4(1.f);
	mat4		m_matViewProj = mat4(1.f);

	std::mutex	m_mutexSize;
	float		m_pixelRatio = 1.f;

	ImagePtr 	m_colorBuffer;
	ImagePtr 	m_albedoBuffer;
	ImagePtr 	m_normalBuffer;
	ImagePtr	m_renderMap;
	RectI		m_renderMapRect = { 0, 0, 0, 0 };
	std::mutex	m_mutexRenderMap;

	bool		m_supportTextureNonPowerOfTwo = false;
	bool		m_supportTextureFloat = false;
	GLint		m_maxTextureSize = 1024;

	GLuint		m_bgTexture = 0;

	GLuint		m_rmTexture = 0;
	QSize		m_rmTexSize = { 0, 0 };
	vec2		m_rmTexCoords = vec2(0.f);
	std::atomic<bool> m_rmUpdateTexture = false;

	void initializeGL();
	void destroyGL();
	void resizeGL(int width, int height);

	void drawTransparentBackground() const;
	void drawAspectBlinds() const;

	GLsizei getTextureSize(GLsizei size) const;
	void updateTexture(GLuint & texture, QSize & texSize, vec2 & texCoords, const IImage * image, const RectI & rc);

	void updateRenderMap();

private slots:
	void onNodeChanged(const INode * node, eNodeChanged what);
	void onCameraChanged();
	void onMaterialChanged();

	void onSceneLock(eSceneInternalModification sm);
	void onSceneUnlock(eSceneInternalModification sm);

public slots:
	void onSelectionChanged();

public:
	CoreRenderer(CoreInstance &core, float devicePixelRatio, bool offscreen);
	~CoreRenderer() override;

	void update() { QQuickFramebufferObject::Renderer::update(); }
	void invalidate() override;

	QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
	void render() override;
	void synchronize(QQuickFramebufferObject *item) override;

	void onLoadingDone();

	void enablePreview(bool b) override;

	void switchToDummyRenderer();

	eGizmo getGizmo() const override;
	void setGizmo(eGizmo gizmo) override;

	bool getShowNormal() const override { return m_showNormal; }
	void setShowNormal(bool b) override;

	bool getEnableDenoise() const override { return m_enableDenoise; }
	void setEnableDenoise(bool b) override { m_enableDenoise = b; }

	int getMaxIterations() const override { return m_maxIterations; }
	void setMaxIterations(int n) override;

	IGeometry * getGeometry(int x, int y) const override;

	const QSize & getSize() override { return m_size; }
	const vec2 & aspectScale() const { return m_aspectScale; }
	float getScale(const vec3 & pos) const;
	bool getScreenPos(vec2 & out, const vec3 & pos) const;
	vec3 getFrustumPosition(int x, int y, float z) const;
	float getX(int x) const;
	float getY(int y) const;

	ImagePtr createScreenshot(int width, int height) override;
	void updateFrame() override;

	QOpenGLFramebufferObject * getFramebufferObject() const override;

	void hoverMoveEvent(QHoverEvent *event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void wheelEvent(QWheelEvent * event) override;

	void threadFunc();
};
