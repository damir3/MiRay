/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#include "../ImageImpl.h"
#include "../Materials/MaterialImpl.h"
#include "../Materials/BSDF.h"
#include "../Samplers/Sobol.h"
#include "../Samplers/Halton.h"
#include "BaseRenderer.h"

// ------------------------------------------------------------------------ //

BaseRenderer::BaseRenderer(Scene & scene)
	: m_numCPU(1)
	, m_scene(scene)
	, m_environmentLight(scene)
	, m_rtcScene(nullptr)
	, m_ambientOcclusion(1.f)
	, m_numAmbientOcclusionSamples(1)
	, m_lightCount(0)
	, m_lightPickProbability(0.f)
	, m_frameBuffer(nullptr)
	, m_normalBuffer(nullptr)
	, m_depthBuffer(nullptr)
	, m_objectsBuffer(nullptr)
	, m_materialsBuffer(nullptr)
	, m_layerBuffers(nullptr)
	, m_projector(nullptr)
	, m_iterationIndex(0)
	, m_frameBlend(0.f)
	, m_maxIntensity(FLT_MAX)
	, m_photonScale(1.f)
	, m_environmentIntensity(1.f)
	, m_invGamma(1.f)
	, m_dofEnabled(false)
	, m_bgScale(0.5f)
	, m_numTilesX(0)
	, m_numTiles(0)
	, m_tileWidth(16)
	, m_tileHeight(16)
	, m_rayLength(1.f)
	, m_distEpsilon(0.001f)
	, m_maxPathLength(20)
	, m_russianRouletteStart(3)
	, m_numBumpLinearSteps(32)
	, m_numBumpBinarySteps(5)
	, m_renderMutex(QMutex::NonRecursive)
//	, m_semaphore(1)
{
	m_frameCount = 0;
	m_interrupted = false;
	m_rayCounter = 0;
	m_tileCounter = 0;

	m_numCPU = QThread::idealThreadCount();
}

BaseRenderer::~BaseRenderer()
{
	interrupt();
	QMutexLocker renderLocker(&m_renderMutex);
}

// ------------------------------------------------------------------------ //

void BaseRenderer::restart(bool forceInterrupt)
{
	if (forceInterrupt || m_frameCount > 0) {
		m_interrupted = true;
		interrupt();
	}

	m_frameCount = 0;
}

// ------------------------------------------------------------------------ //

RectI BaseRenderer::renderFrame(IImage & frameBuffer, IImage * albedoBuffer, IImage * normalBuffer)
{
	QMutexLocker locker(m_scene.mutex());

	if (!m_scene.isValid() || !m_scene.camera().rayProjector())
		return RectI(0, 0, 0, 0); // skip this frame because the scene is not ready

	RenderArea renderArea;
	renderArea.frameSize.x = frameBuffer.width();
	renderArea.frameSize.y = frameBuffer.height();
	const int previewFramesCount = getPreviewFramesCount();
	const int frameCount = m_frameCount;
	if (frameCount < previewFramesCount) {
		renderArea.frameSize.x >>= (previewFramesCount - frameCount);
		renderArea.frameSize.y >>= (previewFramesCount - frameCount);
	}

	renderArea.tilePos = ivec2(0);
	renderArea.tileSize = renderArea.frameSize;

	//auto time1 = std::chrono::high_resolution_clock::now();

	const float MAX_INTENSITY = 8.f;
	const float PHOTON_SCALE = 1.f;
	m_rayCounter = 0;
	if (frameCount < previewFramesCount)
		runIteration(frameBuffer, albedoBuffer, normalBuffer, nullptr, nullptr, nullptr, nullptr, &renderArea, vec2(0.5f, 0.5f), 0, true, MAX_INTENSITY, PHOTON_SCALE);
	else
		runIteration(frameBuffer, albedoBuffer, normalBuffer, nullptr, nullptr, nullptr, nullptr, &renderArea, vec2(m_random.generate1D(), m_random.generate1D()), frameCount - previewFramesCount, true, MAX_INTENSITY, PHOTON_SCALE);

	if (m_interrupted)
		return RectI(0, 0, 0, 0); // skip this frame because it was interrupted

	//auto time2 = std::chrono::high_resolution_clock::now();
	//auto dtime = std::chrono::duration<double>(time2 - time1).count();
	//LogInformation() << QString().asprintf("%d (%dx%d): %.1f ms, %.3fM rps", frameCount, viewport.width(), viewport.height(), dtime * 1e3, m_rayCounter * 1e-6 / dtime);

	m_frameCount++;

	return RectI(0, 0, renderArea.frameSize.x, renderArea.frameSize.y);
}

// ------------------------------------------------------------------------ //

class RenderTask : public QRunnable
{
	BaseRenderer & m_renderer;

public:
	RenderTask(BaseRenderer & renderer) : m_renderer(renderer) {}

    void run()
	{
		m_renderer.threadFunc();
    }
};

void BaseRenderer::runIteration(IImage & frameBuffer, IImage * albedoBuffer, IImage * normalBuffer, IImage * depthBuffer,
								IImage * objectsBuffer, IImage * materialsBuffer, const std::vector<IImage *> * layerBuffers,
								const RenderArea * renderArea, const vec2 & pixelOffset, int iterationIndex,
								bool previewMode, float maxIntensity, float photonScale)
{
	assert(frameBuffer.dataType() == eImageDataType::Float);
	assert(!albedoBuffer || (albedoBuffer->dataType() == eImageDataType::Float && albedoBuffer->format() == eImageFormat::RGB));
	assert(!normalBuffer || (normalBuffer->dataType() == eImageDataType::Float && normalBuffer->format() == eImageFormat::RGB));
	assert(!depthBuffer || (depthBuffer->dataType() == eImageDataType::Float && depthBuffer->format() == eImageFormat::Grayscale));
	assert(!objectsBuffer || (objectsBuffer->dataType() == eImageDataType::Float && objectsBuffer->format() == eImageFormat::RGB));
	assert(!materialsBuffer || (materialsBuffer->dataType() == eImageDataType::Float && materialsBuffer->format() == eImageFormat::RGB));

	QMutexLocker sceneLocker(m_scene.mutex());

	m_scene.updateAnimation(iterationIndex);

	m_interrupted = false;
	m_rtcScene = m_scene.rtcScene();
	auto & camera = m_scene.camera();
	m_projector = camera.rayProjector();
	m_rayLength = camera.farZ().get();
	const auto cameraDistance = camera.depthOfField().get() ? camera.focusDistance().get() : camera.distance().get();
	m_distEpsilon = std::max(cameraDistance * 5e-5f, camera.farZ().get() * 1e-6f);
	m_frameBuffer = &frameBuffer;
	m_normalBuffer = normalBuffer && normalBuffer->format() == eImageFormat::RGB && normalBuffer->dataType() == eImageDataType::Float ? normalBuffer : nullptr;
	m_depthBuffer = depthBuffer && depthBuffer->format() == eImageFormat::Grayscale && depthBuffer->dataType() == eImageDataType::Float ? depthBuffer : nullptr;
	m_albedoBuffer = albedoBuffer && albedoBuffer->format() == eImageFormat::RGB && albedoBuffer->dataType() == eImageDataType::Float ? albedoBuffer : nullptr;
	m_objectsBuffer = objectsBuffer && objectsBuffer->format() == eImageFormat::RGB && objectsBuffer->dataType() == eImageDataType::Float ? objectsBuffer : nullptr;
	m_materialsBuffer = materialsBuffer && materialsBuffer->format() == eImageFormat::RGB && materialsBuffer->dataType() == eImageDataType::Float ? materialsBuffer : nullptr;
	m_layerBuffers = layerBuffers && layerBuffers->size() > 0 ? layerBuffers : nullptr;
	if (m_layerBuffers) { // check if some render layer buffer has incorrect format
		for (auto buffer : *m_layerBuffers) {
			if (!buffer || buffer->format() != eImageFormat::RGBA || buffer->dataType() != eImageDataType::Float) {
				m_layerBuffers = nullptr;
				break;
			}
		}
	}

	if (renderArea) {
		m_renderArea = *renderArea;
		m_renderArea.tileSize.x = std::min<int>(m_renderArea.tileSize.x, frameBuffer.width());
		m_renderArea.tileSize.y = std::min<int>(m_renderArea.tileSize.y, frameBuffer.height());
	} else {
		m_renderArea.tilePos.x = m_renderArea.tilePos.y = 0;
		m_renderArea.tileSize.x = m_renderArea.frameSize.x = frameBuffer.width();
		m_renderArea.tileSize.y = m_renderArea.frameSize.y = frameBuffer.height();
	}

	m_iterationIndex = iterationIndex;
	BSDF::g_frameIndex = iterationIndex;
	m_frameBlend = 1.f / (iterationIndex + 1);
	m_maxIntensity = maxIntensity;
	m_photonScale = photonScale;
	m_environmentIntensity = m_scene.environmentIntensity().rgb();
	m_invGamma = 1.f / std::max(camera.gamma().get(), FLT_MIN);

	prepareLights();

	m_dp = vec2(1.f / m_renderArea.frameSize.x, 1.f / m_renderArea.frameSize.y);
	m_po = pixelOffset * m_dp;
	auto screenAspect = (float)m_renderArea.frameSize.x / (float)m_renderArea.frameSize.y;
	auto cameraAspect = previewMode ? camera.aspect().get() : screenAspect;
	camera.setRenderAspect(cameraAspect); // set temporary camera aspect
	if (screenAspect < cameraAspect) {
		auto scale = cameraAspect / screenAspect;
		m_dp.y *= scale;
		m_po.y -= scale * 0.5f -  0.5f;
	} else if (screenAspect > cameraAspect) {
		auto scale = screenAspect / cameraAspect;
		m_dp.x *= scale;
		m_po.x -= scale * 0.5f -  0.5f;
	}

	m_bgMode = (eBackgroundMode)m_scene.backgroundMode().getIndex();
	if (auto bgImage = m_scene.background().texture()->getImage()) {
		auto w = (float)bgImage->width();
		auto h = (float)bgImage->height() * cameraAspect;
		m_bgScale = w < h ? vec2(0.5f, 0.5f * w / h) : vec2(0.5f * h / w, 0.5f);
	} else
		m_bgScale = vec2(0.5f);

	m_floor.enabled = m_scene.floorEnabled().get();
	m_floor.level = -m_distEpsilon;
	m_floor.reflectionLevel = m_scene.floorReflectionLevel().value();
	m_floor.roughness = m_scene.floorRoughness().value();
	m_floor.shadowLevel = m_scene.floorShadowLevel().value();
	if (m_floor.shadowLevel < 1.f)
		m_floor.shadowLevel = 1.f - ColorUtils::sRGBToLinear(1.f - m_floor.shadowLevel);
	m_floor.diffuseIntensity = m_scene.diffuseIntensity().rgb() * 0.5f;

	prepareDepthOfField(iterationIndex > 0);

	m_tileCounter = 0;
	m_numTilesX = (m_renderArea.tileSize.x + m_tileWidth - 1) / m_tileWidth;
	int numTilesY = (m_renderArea.tileSize.y + m_tileHeight - 1) / m_tileHeight;
	m_numTiles = m_numTilesX * numTilesY;

	QMutexLocker renderLocker(&m_renderMutex);

	beginIteration();

	const auto numThreads = std::max<int>(m_numCPU - 1, 1);

	if (!m_interrupted) {
		std::vector< std::future<void> > ftasks;
		for (int i = 0; i < numThreads; i++)
			ftasks.emplace_back(std::async(std::launch::async, &BaseRenderer::threadFunc, this));

		for (auto & rt: ftasks)
			rt.wait();
	}

	endIteration();

	// restore camera aspect
	camera.setRenderAspect(camera.aspect().get());
}

void BaseRenderer::prepareLights()
{
	m_lights.clear();
	m_infiniteLights.clear();
	m_scene.root().findLights(m_lights, true);

	m_lightCount = static_cast<int>(m_lights.size());
	if (!isBlack(m_scene.environment().rgb() * m_environmentIntensity)) {
		m_lightCount++;
		m_infiniteLights.push_back(&m_environmentLight);
	}

	m_lightPickProbability = 1.f / m_lightCount;

	m_environmentLight.prepare();
	for (auto light : m_lights) {
		light->prepare();
		if (!light->isFinite())
			m_infiniteLights.push_back(light);
	}
}

void BaseRenderer::prepareDepthOfField(bool enable)
{
	auto & camera = m_scene.camera();
	m_dofEnabled = camera.depthOfField().get() && enable;
	if (m_dofEnabled) {
		auto numBlades = camera.diaphragmBlades().get();
		auto aspect = 1.f / camera.aspect().get();
		if (numBlades > 2 && numBlades < 32) { // polygonal bokeh
			auto dAngle = M_2PIf / camera.diaphragmBlades().get();
			for (auto it = std::begin(m_dofOffset); it != std::end(m_dofOffset); ++it) {
				int n = m_random.iRand() % camera.diaphragmBlades().get();
				auto a1 = n * dAngle + glm::radians(camera.bokehRotation().get()), a2 = a1 + dAngle;
				vec2 d1(std::sin(a1) * aspect, std::cos(a1));
				vec2 d2(std::sin(a2) * aspect, std::cos(a2));
				auto p = uniformSampleTriangle(m_random);
				*it = d1 * p.x + d2 * p.y;
			}
		} else { // circular bokeh
			for (auto it = std::begin(m_dofOffset); it != std::end(m_dofOffset); ++it) {
				*it = uniformSampleDisk(m_random);
				it->x *= aspect;
			}
		}
	}
}

void BaseRenderer::interrupt()
{
	m_tileCounter = m_numTiles;
}

// ------------------------------------------------------------------------ //

bool BaseRenderer::getNextTile(RectI & rc)
{
	auto pos = m_tileCounter++;
	if (pos >= m_numTiles)
		return false;

	rc.left = m_renderArea.tilePos.x + m_tileWidth * (pos % m_numTilesX);
	rc.top = m_renderArea.tilePos.y + m_tileHeight * (pos / m_numTilesX);
	rc.right = std::min(rc.left + m_tileWidth, m_renderArea.tilePos.x + m_renderArea.tileSize.x);
	rc.bottom = std::min(rc.top + m_tileHeight, m_renderArea.tilePos.y + m_renderArea.tileSize.y);
	return true;
}

static const vec4 ZERO_COLOR(0.f, 0.f, 0.f, 0.f);

void BaseRenderer::renderTile(const RectI & rc, Sampler & sampler)
{
	auto & bg = m_scene.background();
	for (int y = rc.top; y < rc.bottom; y++) {
		vec2 p;
		p.y = y * m_dp.y + m_po.y;

		for (int x = rc.left; x < rc.right; x++) {
			p.x = x * m_dp.x + m_po.x;

			const int px = x - m_renderArea.tilePos.x;
			const int py = y - m_renderArea.tilePos.y;

			sampler.init(x, y, m_iterationIndex);

			vec4 color;
			vec3 origin, dir;
			float rayLength = 1.f;
			TraceResult res;

			if (m_projector->getRay(origin, dir, p, m_dofEnabled ? &m_dofOffset[(x & 31) + ((y & 31) << 5)] : nullptr)) {
				res.depth = rayLength = glm::length(dir);
				if (rayLength >= 0.f) dir *= 1.f / rayLength;
				TraceContext ctx(origin, dir, rayLength, sampler, y * m_renderArea.frameSize.x + x);
				ctx.ray.ray.id = x + (y << 16);
				traceCameraPath(res, ctx);
				assert(isfinite(res.color.r) && isfinite(res.color.g) && isfinite(res.color.b));
				assert(isfinite(res.opacity.r) && isfinite(res.opacity.g) && isfinite(res.opacity.b));

				float intensity = maxComponent(res.color);
				if (intensity > m_maxIntensity)
					res.color *= (m_maxIntensity / intensity);

				res.opacity = glm::clamp(res.opacity, 0.f, 1.f);

				auto opacity = maxComponent(res.opacity);

				if (minComponent(res.opacity) < 1.f) {
					if (m_bgMode != BackgroundMode_Transparent) {
						vec4 bgColor;
						vec2 sp;
						switch (m_bgMode) {
							case BackgroundMode_Environment:
								bgColor = vec4(m_environmentLight.getRadiance(ctx.ray, nullptr), 1.f);
								break;

							case BackgroundMode_PlaneImage: {
								if (bg.texture()->valid() && m_projector->getViewportCoords(sp, ctx.ray.target(m_rayLength), false))
									bgColor = bg.getColor4(sp * m_bgScale + 0.5f);
								else
									bgColor = bg.rgba();
								break;
							}

							case BackgroundMode_SphericalImage:
								if (bg.texture()->valid())
									bgColor = bg.getColor4(sphericalTexCoords(ctx.ray.direction()));
								else
									bgColor = bg.rgba();
								break;

							case BackgroundMode_RadialGradient: {
								float factor = glm::clamp(1.4241f * glm::length(p - vec2(0.5f)), 0.f, 1.f);
								bgColor = glm::mix(bg.rgba(), m_scene.backgroundColor2().rgba(), factor);
								if (bg.texture()->valid() && m_projector->getViewportCoords(sp, ctx.ray.target(m_rayLength), false))
									bgColor *= bg.texture()->getColor4(sp * m_bgScale + 0.5f);
								break;
							}

							case BackgroundMode_VerticalGradient: {
								float factor = glm::clamp(p.y, 0.f, 1.f);
								bgColor = glm::mix(bg.rgba(), m_scene.backgroundColor2().rgba(), factor);
								if (bg.texture()->valid() && m_projector->getViewportCoords(sp, ctx.ray.target(m_rayLength), false))
									bgColor *= bg.texture()->getColor4(sp * m_bgScale + 0.5f);
								break;
							}

							case BackgroundMode_HorizontalGradient: {
								float factor = glm::clamp(p.x, 0.f, 1.f);
								bgColor = glm::mix(bg.rgba(), m_scene.backgroundColor2().rgba(), factor);
								if (bg.texture()->valid() && m_projector->getViewportCoords(sp, ctx.ray.target(m_rayLength), false))
									bgColor *= bg.texture()->getColor4(sp * m_bgScale + 0.5f);
								break;
							}

							default:
								bgColor = bg.rgba();
								break;
						}

						if (opacity > EPS_COLOR && res.geometry != nullptr) { // apply absoption color fix
							auto weight = (vec3(1.f) - res.opacity) * opacity;
							res.color += weight * reinterpret_cast<const vec3 &>(bgColor);
							if (bgColor.a < 1.f)
								res.color += weight * m_scene.environment().rgb() * (m_environmentIntensity * (1.f - bgColor.a));
						}

						if (opacity < 1.f) { // blend background
							bgColor.a *= (1.f - opacity);
							res.color += reinterpret_cast<const vec3 &>(bgColor) * bgColor.a;
							opacity += bgColor.a;
						}
					} else if (opacity > EPS_COLOR && res.geometry != nullptr) { // apply absoption color fix for transparent background (ignore floor shadow)
						auto weight = vec3(1.f) - res.opacity;
						res.color += weight * m_scene.environment().rgb() * (m_environmentIntensity * opacity);
					}
				}

				if (m_invGamma != 1.f)
					res.color = glm::pow(res.color, vec3(m_invGamma));

				color = vec4(res.color, opacity);
			} else
				color = ZERO_COLOR;

			if (m_frameBlend < 1.f) {
				const auto srcColor = m_frameBuffer->getPixel(px, py);
				m_frameBuffer->setPixel(px, py, glm::mix(srcColor, color, m_frameBlend));

				if (m_normalBuffer) {
					const auto srcNormal = m_normalBuffer->getPixelColor(px, py);
					res.normal = glm::transformNormal(res.normal, m_scene.camera().viewMatrix());
					m_normalBuffer->setPixel(px, py, vec4(glm::mix(srcNormal, res.normal * 0.5f + 0.5f, m_frameBlend), 1.f));
				}

				if (m_depthBuffer) {
					auto depth = m_depthBuffer->getPixelColor(px, py).x;
//					depth = std::min(depth, std::min(res.depth / rayLength, 1.f));
					depth = lerp(depth, std::min(res.depth / rayLength, 1.f), m_frameBlend);
					m_depthBuffer->setPixel(px, py, vec4(depth, depth, depth, 1.f));
				}

				if (m_albedoBuffer) {
					m_albedoBuffer->setPixel(px, py, vec4(glm::mix(m_albedoBuffer->getPixelColor(px, py), res.albedo, m_frameBlend), 1.f));
				}

				if (m_objectsBuffer) {
					m_objectsBuffer->setPixel(px, py, vec4(glm::mix(m_objectsBuffer->getPixelColor(px, py), res.objectColor(), m_frameBlend), 1.f));
				}

				if (m_materialsBuffer) {
					m_materialsBuffer->setPixel(px, py, vec4(glm::mix(m_materialsBuffer->getPixelColor(px, py), res.materialColor(), m_frameBlend), 1.f));
				}

				if (m_layerBuffers) {
					const auto renderLayer = res.renderLayer();
					for (int i = 0, c = (int)m_layerBuffers->size(); i < c; i++) {
						auto * buffer = (*m_layerBuffers)[i];
						buffer->setPixel(px, py, glm::mix(buffer->getPixel(px, py), renderLayer == i ? color : ZERO_COLOR, m_frameBlend));
					}
				}
			} else {
				m_frameBuffer->setPixel(px, py, color);
				if (m_normalBuffer) {
					res.normal = glm::transformNormal(res.normal, m_scene.camera().viewMatrix());
					m_normalBuffer->setPixel(px, py, vec4(res.normal * 0.5f + 0.5f, 1.f));
				}

				if (m_depthBuffer) {
					const auto depth = std::min(res.depth / rayLength, 1.f);
					m_depthBuffer->setPixel(px, py, vec4(depth, depth, depth, 1.f));
				}

				if (m_albedoBuffer) {
					m_albedoBuffer->setPixel(px, py, vec4(res.albedo, 1.f));
				}

				if (m_objectsBuffer) {
					m_objectsBuffer->setPixel(px, py, vec4(res.objectColor(), 1.f));
				}

				if (m_materialsBuffer) {
					m_materialsBuffer->setPixel(px, py, vec4(res.materialColor(), 1.f));
				}

				if (m_layerBuffers) {
					const auto renderLayer = res.renderLayer();
					for (int i = 0, c = (int)m_layerBuffers->size(); i < c; i++) {
						auto * buffer = (*m_layerBuffers)[i];
						buffer->setPixel(px, py, renderLayer == i ? color : ZERO_COLOR);
					}
				}
			}
		}
	}
}

void BaseRenderer::threadFunc()
{
	QThread::currentThread()->setPriority(QThread::LowestPriority);

	HaltonSampler sampler;
	sampler.setResolution(m_renderArea.frameSize.x, m_renderArea.frameSize.y);
	RectI rc;
	while (getNextTile(rc))
		renderTile(rc, sampler);

//	if (--m_taskCount == 0)
//		m_semaphore.release();
}

// ------------------------------------------------------------------------ //

float BaseRenderer::evaluateShadowOpacity(TraceResult & res, TraceContext & ctx, const BoundaryStack & boundaryStack, VolumeSegments & volumeSegments)
{
	auto light = getLight(size_t(ctx.sampler.generate1D() * m_lightCount));
	assert(light);

	ctx.ray.hitPoint = ctx.ray.target(-ctx.ray.origin().z / ctx.ray.direction().z);

	// Sample light
	vec3 directionToLight;
	float distanceToLight, directIllumPdfW;
	vec3 radiance = light->illuminateFloor(ctx.ray.hitPoint, ctx.sampler, directionToLight, distanceToLight, directIllumPdfW);

	if (directionToLight.z > 0.f && !isBlack(radiance) &&
		occluded(ctx.ray.hitPoint, directionToLight, distanceToLight, ctx.ray, ctx.sampler, boundaryStack, 0, volumeSegments)) {
		float opacity = luminance(radiance) * (m_floor.shadowLevel / (M_PIf * m_lightPickProbability * directIllumPdfW));
		return std::min(opacity, 1.f);
	}

	return 0.f;
}

// ------------------------------------------------------------------------ //

bool BaseRenderer::handleVolumeSegment(Ray & oRay, Isect & oResult, VolumeSegments * oVolumeSegments, Sampler & sampler,
									   MaterialImpl * currentMediumPtr, float distMin, float distMax,
									   const float originStart, const float originEnd,
									   const uint32_t aRaySamplingFlags, const bool sampleMedia) const
{
	assert(distMin < distMax);

	if (!currentMediumPtr) return false;

	float raySamplePdf = 1.f;
	float raySampleRevPdf = 1.f;

	bool scatteringOccured = false;

	if (currentMediumPtr->hasScattering()) {
		uint raySamplingFlags = 0;
		if ((aRaySamplingFlags & kOriginInMedium) != 0 && distMin == originStart)	raySamplingFlags |= kOriginInMedium;
		if ((aRaySamplingFlags & kEndInMedium) != 0    && distMax == originEnd)		raySamplingFlags |= kEndInMedium;

		if (sampleMedia) {
			// To avoid sampling zero direction
			float uSample = sampler.generate1D();
			while (uSample == 1.f) uSample = sampler.generate1D();

			// Sample ray within the bounds of the segment
			const auto distToNextSegment = distMax - distMin;
			auto distToMedium = currentMediumPtr->sampleRay(distToNextSegment, uSample, &raySamplePdf, raySamplingFlags, &raySampleRevPdf);

			// Output medium hit point if sampled inside medium
			if (distToMedium < distToNextSegment) {
				scatteringOccured = true;
				distMax = distMin + distToMedium;

				oRay.ray.tnear = originStart;
				oRay.ray.tfar = distMax;
				oRay.hitPoint = oRay.target(oRay.ray.tfar);

				oResult.mDist = distMax;
				oResult.mMaterial = nullptr;
				oResult.mMedium = currentMediumPtr;
				oResult.mLight = nullptr;
				oResult.mGeometry = nullptr;
				oResult.mEnter = false;
				oResult.mFloor = false;
			}
		} else {
			raySamplePdf = currentMediumPtr->raySamplePdf(distMin, distMax, raySamplingFlags, &raySampleRevPdf);
		}

		assert(raySamplePdf > 0);
		assert(raySampleRevPdf > 0);
	}

	if (oVolumeSegments && oVolumeSegments->size() < oVolumeSegments->capacity()) {
		VolumeSegment segment;
		segment.mDistMin = distMin;
		segment.mDistMax = distMax;
		segment.mRaySamplePdf = raySamplePdf;
		segment.mRaySampleRevPdf = raySampleRevPdf;
		segment.mAttenuation = currentMediumPtr->evalAttenuation(segment.mDistMin, segment.mDistMax);
		segment.mEmission = currentMediumPtr->evalEmission(segment.mDistMin, segment.mDistMax);
		segment.mMedium = currentMediumPtr;
		oVolumeSegments->push_back(segment);
	}

	return scatteringOccured;
}

bool BaseRenderer::checkMaterialRefraction(MaterialImpl * material, const BoundaryStack & boundaryStack, bool backface)
{
	if (material->isEmpty())
		return false;

	if (material->isOpaque())
		return true;

	auto iorEnter = boundaryStack.Top().mIOR;
	auto iorExit = backface ?
		(boundaryStack.Top().mMedium != material ? boundaryStack.Top().mIOR : boundaryStack.SecondFromTop().mIOR) :
		&material->indexOfRefraction();
	return iorEnter != iorExit && getIOR(iorEnter).real() != getIOR(iorExit).real();
}

void BaseRenderer::updateBoundaryStackOnRefract(const Isect & aIsect, BoundaryStack & oBoundaryStack)
{
	if (aIsect.mEnter) {
		if (aIsect.mGeometry) {
			auto material = aIsect.mGeometry->material();
			assert(material);
			if (!material->isEmpty())
				oBoundaryStack.Push(StackElement(material), material->getMediumBoundaryPriority());
		}
	} else {
		if (oBoundaryStack.Size() > 1)
			oBoundaryStack.PopTop();
	}
}

static float distPlusEpsilon(float dist, float epsilon)
{
	float res = dist + epsilon;
	return res > dist ? res : dist * 1.00001f;
	//return res > dist ? res : std::nextafter(dist, dist + 1000.f);
}

static float distMinusEpsilon(float dist, float epsilon)
{
	float res = dist - epsilon;
	return res < dist ? res : dist * 0.99999f;
	//return res < dist ? res : std::nextafter(dist, dist - 1000.f);
}

bool BaseRenderer::intersect(Ray & ray,
							 Isect & oResult,
							 Sampler & sampler,
							 BoundaryStack & oBoundaryStack,
							 const uint32_t aOptions,
							 const uint32_t aRaySamplingFlags,
							 VolumeSegments * oVolumeSegmentsToIsect) const
{
	assert(!oBoundaryStack.IsEmpty());

	const bool ignoreMedia = (aOptions & kIgnoreMediaAltogether) != 0;
	const bool testOcclusion = (aOptions & kOcclusionTest) != 0;
	const bool sampleMedia = (ignoreMedia || testOcclusion) ? false : (aOptions & kSampleVolumeScattering) != 0;

	// Current t max. In case of occlusion test we subtract two epsilons, one at the beginning of the ray because of origin offset,
	// one at the end in order not to accidentally hit the target surface
	if (testOcclusion)
		ray.ray.tfar = distMinusEpsilon(ray.ray.tfar, m_distEpsilon);

	bool hitFloor;
	if (m_floor.enabled) {
		hitFloor = false;
		if (ray.origin().z > m_floor.level) { // over the floor
			if (ray.direction().z < 0.f) {
				float tfar = (m_floor.level - ray.origin().z) / ray.direction().z;
				hitFloor = (ray.ray.tfar > tfar);
				ray.ray.tfar = std::min(ray.ray.tfar, tfar);
			}
		} else {// under the floor
			if (ray.direction().z > 0.f)
				ray.ray.tnear = std::max(ray.ray.tnear, (m_floor.level - ray.origin().z) / ray.direction().z);
			else
				ray.ray.tnear = ray.ray.tfar; // no intersection
		}
	}

	const auto distMin = ray.ray.tnear;
	const auto distMax = ray.ray.tfar;

	auto segmentDistMin = distMin;
	bool hit = false;

	if ((aRaySamplingFlags & kOriginInMedium) == 0) // is on surface
		ray.ray.tnear = distPlusEpsilon(ray.ray.tnear, m_distEpsilon);

	for (uint i = 0; i < m_maxPathLength; i++) {
		// Try to find intersection. We use origin epsilon offset for numerical stable intersection, but we immediately
		// restore real origin and distance afterwards for clarity

		if (ray.ray.tnear >= ray.ray.tfar) {
			hit = false;
			break;
		}

		if (ray.normalMap) { // has normalmap
			if (testOcclusion) {
				if (!traceBumpShadow(ray.direction(), ray, m_numBumpLinearSteps >> 1, sampler.generate1D()))
					return true; // ray is occluded by the bump

				{
					ray.normalMap = nullptr;
					ray.hitBoth = true;
					intersect(ray);
					ray.hitBoth = false;

					if (ray.hit.geomID != RTC_INVALID_GEOMETRY_ID && ray.hit.primID != RTC_INVALID_GEOMETRY_ID && glm::dot(ray.direction(), ray.Ng()) > 0.f) { // hit geometry
						ray.ray.tnear = distPlusEpsilon(ray.ray.tfar, m_distEpsilon);
						ray.ray.tfar = distMax;
						intersect(ray);
					}
				}
			} else {
				if (traceBumpInside(ray.direction(), ray, m_numBumpLinearSteps >> 1, m_numBumpBinarySteps, sampler.generate1D())) { // ray doesn't intersect the bump
					ray.normalMap = nullptr;
					ray.hitBoth = true;
					intersect(ray);
					ray.hitBoth = false;

					if (ray.hit.geomID != RTC_INVALID_GEOMETRY_ID && ray.hit.primID != RTC_INVALID_GEOMETRY_ID && glm::dot(ray.direction(), ray.Ng()) > 0.f) { // hit geometry
						ray.ray.tnear = distPlusEpsilon(ray.ray.tfar, m_distEpsilon);
						ray.ray.tfar = distMax;
						intersect(ray);
					}
				}
			}
		} else {
			intersect(ray);
		}

		hit = ray.hit.geomID != RTC_INVALID_GEOMETRY_ID;
		if (!hit)
			break;

		auto geom = ray.geom;
		if (!geom) { // hit some light
			oResult.mDist = ray.ray.tfar;
			oResult.mMaterial = nullptr;
			oResult.mMedium = nullptr;
			oResult.mLight = LightNode::getByRay(m_rtcScene, ray);
			oResult.mGeometry = nullptr;
			oResult.mEnter = false;
			oResult.mFloor = false;

			ray.updateLightIntersection();
			ray.node = oResult.mLight;
			break;
		}

		auto material = geom->material();
		assert(material);

		if (!ray.normalMap) {
			ray.updateGeometryIntersection();

			{// hit point correction
				auto v0 = glm::transformCoord(ray.verts[0]->pos, ray.node->globalTransformation());
				auto errorDist = glm::dot(v0 - ray.hitPoint, ray.geomNormal);
				if (errorDist != 0.f)
					ray.hitPoint += ray.geomNormal * errorDist;
			}

			if (material->hasBump())
				traceBumpMap(ray, material->bump(), m_numBumpLinearSteps, m_numBumpBinarySteps);
		}

		const auto priority = material->getMediumBoundaryPriority();
		StackElement element(material);

		MaterialLayerImpl *layer = nullptr;
		if (priority >= oBoundaryStack.TopPriority()) {
			if (ray.hitBack && material->doubleSided().get() && (material->isEmpty() || material->isOpaque())) { // flip hit side
				ray.hitBack = false;
				ray.normal = -ray.normal;
				ray.geomNormal= -ray.geomNormal;
			}

			layer = BSDF::findLayer(material, ray, sampler);
			if (!layer && material->isEmpty()) { // ignore hit, search next
				ray.ray.tnear = distPlusEpsilon(ray.ray.tfar, m_distEpsilon);
				ray.ray.tfar = distMax;
				hit = false;
				continue;
			}
		}

		if (!ignoreMedia) {
			if (!ray.hitBack) { // enter
				if (priority >= oBoundaryStack.TopPriority()) {
					if (handleVolumeSegment(ray, oResult, oVolumeSegmentsToIsect, sampler,
											oBoundaryStack.Top().mMedium, segmentDistMin, ray.ray.tfar,
											distMin, ray.ray.tfar, aRaySamplingFlags, sampleMedia))
						return true; // scattering occured

					segmentDistMin = ray.ray.tfar;
				}
			} else {//exit
				if (priority == oBoundaryStack.TopPriority() && element == oBoundaryStack.Top()) {
					if (handleVolumeSegment(ray, oResult, oVolumeSegmentsToIsect, sampler,
											oBoundaryStack.Top().mMedium, segmentDistMin, ray.ray.tfar,
											distMin, ray.ray.tfar, aRaySamplingFlags, sampleMedia))
						return true; // scattering occured

					segmentDistMin = ray.ray.tfar;
				}
			}
		}

		if (priority >= oBoundaryStack.TopPriority()) {
			if (layer || checkMaterialRefraction(material, oBoundaryStack, ray.hitBack)) {
				oResult.mDist = ray.ray.tfar;
				oResult.mMaterial = layer;
				oResult.mMedium = nullptr;
				oResult.mLight = nullptr;
				oResult.mGeometry = geom;
				oResult.mEnter = !ray.hitBack;
				oResult.mFloor = false;
				break;
			}
		}

		if (!ray.hitBack) // enter
			oBoundaryStack.Push(element, priority);
		else if (oBoundaryStack.Size() > 1) // exit
			oBoundaryStack.Pop(element, priority);

		ray.ray.tnear = distPlusEpsilon(ray.ray.tfar, m_distEpsilon);
		ray.ray.tfar = distMax;
		hit = false;
	}

	if (!ignoreMedia && segmentDistMin < ray.ray.tfar) {
		if (handleVolumeSegment(ray, oResult, oVolumeSegmentsToIsect, sampler,
								oBoundaryStack.Top().mMedium, segmentDistMin, ray.ray.tfar,
								distMin, ray.ray.tfar, aRaySamplingFlags, sampleMedia))
			return true; // scattering occured
	}

	ray.ray.tnear = distMin;

	if (!hit && m_floor.enabled && hitFloor) {
		oResult.mDist = ray.ray.tfar;
		oResult.mMaterial = nullptr;
		oResult.mMedium = nullptr;
		oResult.mLight = nullptr;
		oResult.mGeometry = nullptr;
		oResult.mEnter = true;
		oResult.mFloor = true;
		hit = m_floor.reflectionLevel >= 1.f || (m_floor.reflectionLevel > 0.f && m_floor.reflectionLevel > sampler.generate1D());
		if (hit) {
			*(vec3 *)&ray.hit.Ng_x = vec3(0.f, 0.f, 1.f);
			ray.updateLightIntersection();
		}
	}

	return hit;
}

// ------------------------------------------------------------------------ //

vec3 VolumeSegment::AccumulateAttenuationWithoutPdf(const VolumeSegments &aSegments)
{
	vec3 attenuation(1.f);
	for (auto i = aSegments.cbegin(); i != aSegments.cend(); ++i) {
		attenuation *= i->mAttenuation;
	}
	return attenuation;
}

vec3 VolumeSegment::AccumulateAttenuatedEmissionWithoutPdf(const VolumeSegments &aSegments)
{
	vec3 attenuation(1.f);
	vec3 emission(0.f);
	for (auto i = aSegments.cbegin(); i != aSegments.cend(); ++i) {
		attenuation *= i->mAttenuation;
		emission += i->mEmission *attenuation;
	}
	return emission;
}

vec3 VolumeSegment::AccumulateAttenuatedEmissionWithPdf(const VolumeSegments &aSegments)
{
	vec3 attenuation(1.f);
	vec3 emission(0.f);
	float pdf(1.f);
	for (auto i = aSegments.cbegin(); i != aSegments.cend(); ++i) {
		attenuation *= i->mAttenuation;
		pdf *= i->mRaySamplePdf;
		emission += i->mEmission * attenuation / pdf;
	}
	return emission;
}

vec3 VolumeSegment::AccumulateAttenuatedEmissionWithRevPdf(const VolumeSegments &aSegments)
{
	vec3 attenuation(1.f);
	vec3 emission(0.f);
	float revPdf(1.f);
	for (auto i = aSegments.cbegin(); i != aSegments.cend(); ++i) {
		attenuation *= i->mAttenuation;
		revPdf *= i->mRaySampleRevPdf;
		emission += i->mEmission * attenuation / revPdf;
	}
	return emission;
}

float VolumeSegment::AccumulatePdf(const VolumeSegments &aSegments)
{
	float pdf(1.f);
	for (auto i = aSegments.cbegin(); i != aSegments.cend(); ++i) {
		pdf *= i->mRaySamplePdf;
	}
	return pdf;
}

float VolumeSegment::AccumulateRevPdf(const VolumeSegments &aSegments)
{
	float revPdf(1.f);
	for (auto i = aSegments.cbegin(); i != aSegments.cend(); ++i) {
		revPdf *= i->mRaySampleRevPdf;
	}
	return revPdf;
}
