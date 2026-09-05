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

#pragma once

#include "../EnvironmentLight.h"
#include "../LightNode.h"

#include "StaticArray.h"
#include "PriorityStack.h"

class Scene;
class ICameraProjector;
class IndexOfRefractionImpl;
class MaterialLayerImpl;

enum IntersectOptions {
	/// Act as if there were no participating media in the scene
	kIgnoreMediaAltogether	= 0x0001,

	/// Sample scattering distance in the medium (if not set, always returns the nearest surface as the scattering point)
	kSampleVolumeScattering	= 0x0002,

	kOcclusionTest			= 0x0004,
};

#define THIN_WALL_PRIORITY		-1
#define GLOBAL_MEDIUM_PRIORITY	-2

struct VolumeSegment;
typedef StaticArray<VolumeSegment, 30> VolumeSegments;

// Segment of a ray in one medium
struct VolumeSegment {
	float mDistMin;         // Distance of the segment begin from the ray origin.
	float mDistMax;         // Distance of the segment end from the ray origin.
	float mRaySamplePdf;    // If scattering occurred in this medium: PDF of having samples in this segment; otherwise: PDF of passing through the entire medium.
	float mRaySampleRevPdf; // Similar to mRaySamplePdf but in a reverse direction.
	vec3  mAttenuation;     // Attenuation caused by this segment (not divided by PDF).
	vec3  mEmission;        // Emission coming from this segment (neither attenuated, nor divided by PDF).
	MaterialImpl * mMedium;	// ID of the medium in this segment.

	// Accumulates attenuation from all the given segments (just multiplies them together).
	static vec3 AccumulateAttenuationWithoutPdf(const VolumeSegments &aSegments);

	// Accumulates attenuated emission from all the given segments without dividing by PDF
	// (= attenuation1 * emission1 + attenuation1 * attenuation2 * emission2 + attenuation1 * attenuation2 * attenuation3 * emission3 + ... ).
	static vec3 AccumulateAttenuatedEmissionWithoutPdf(const VolumeSegments &aSegments);

	// Accumulates attenuated emission from all the given segments with dividing by PDF
	// (= attenuation1/pdf1 * emission1 + attenuation1/pdf1 * attenuation2/pdf2 * emission2 + attenuation1/pdf1 * attenuation2/pdf2 * attenuation3/pdf3 * emission3 + ... ).
	static vec3 AccumulateAttenuatedEmissionWithPdf(const VolumeSegments &aSegments);

	// Accumulates attenuated emission from all the given segments with dividing by reverse PDF
	// (= attenuation1/revpdf1 * emission1 + attenuation1/revpdf1 * attenuation2/revpdf2 * emission2 + attenuation1/revpdf1 * attenuation2/revpdf2 * attenuation3/revpdf3 * emission3 + ... ).
	static vec3 AccumulateAttenuatedEmissionWithRevPdf(const VolumeSegments &aSegments);

	// Accumulates PDFs from all the given segments (just multiplies them together).
	static float AccumulatePdf(const VolumeSegments &aSegments);

	// Accumulates reverse PDFs from all the given segments (just multiplies them together).
	static float AccumulateRevPdf(const VolumeSegments &aSegments);
};

// For path tracing ids of material and medium of last hist geometry is stored on the stack
struct StackElement {
	MaterialImpl * mMedium;
	const IndexOfRefractionImpl * mIOR;

	StackElement() {}
	StackElement(MaterialImpl * medium) : mMedium(medium), mIOR(medium ? &medium->indexOfRefraction() : nullptr) {}

	bool operator == (const StackElement & elem) const { return elem.mMedium == mMedium; }
	bool operator != (const StackElement & elem) const { return elem.mMedium != mMedium; }
};

typedef StaticPriorityStack2<StackElement,20> BoundaryStack;

struct Isect {
	float mDist = std::numeric_limits<float>::infinity(); // Distance to the closest intersection on a ray (serves as ray.tmax).
	MaterialLayerImpl * mMaterial = nullptr; // Intersected material, nullptr indicates scattering event inside medium.
	MaterialImpl * mMedium = nullptr; // Interacting medium or medium behind the hit surface, nullptr means that crossing the hit surface does not affect medium.
	const LightNode * mLight = nullptr; // Intersected light, nullptr means none.
	const Geometry * mGeometry = nullptr; // Intersected geometry element - used for calculating shading normal etc.
	bool mEnter = false; // Whether the ray enters geometry at this intersection (cosine of its direction and the normal is negative).
	bool mFloor = false; // Intersected floor

	// True if this Isect represents a scattering event inside a medium.
	bool isInMedium() const { return mMedium != nullptr; }

	// True if this Isect represents an intersection with geometry.
	bool isOnSurface() const { return mMedium == nullptr; }

	// Whether properties of this Isect have valid values.
	bool isValid() const { return mDist > 0.f && (mGeometry || mMedium || mLight || mFloor); }

	// Enables sorting Isects according their distance on a ray.
	bool operator < (const Isect & right) const { return mDist < right.mDist; }
};

struct TraceContext {
	Ray   ray;
	Sampler & sampler;
	const IndexOfRefractionImpl * ior;
	const int pathIndex;
	bool  reflected;
	bool  floor;
	vec3  floorRayOrigin;
	vec3  floorRayDirection;

	TraceContext(const vec3 & origin, const vec3 & ndir, float rayLength, Sampler & sampler, int pi)
		: sampler(sampler)
		, ior(nullptr)
		, pathIndex(pi)
		, reflected(false)
		, floor(false)
	{
		ray.init(origin, ndir, rayLength);
	}
};

struct Floor {
	bool  enabled = false;
	float level = 0.f;
	float reflectionLevel = 0.f;
	float roughness = 0.f;
	float shadowLevel = 0.f;
	vec3  diffuseIntensity = vec3(1.f);
};

class BaseRenderer
{
protected:
	int					m_numCPU;
	std::atomic_int		m_frameCount;
	std::atomic_bool	m_interrupted;
	Scene &				m_scene;
	EnvironmentLight	m_environmentLight;

	RTCScene	m_rtcScene;
	float		m_ambientOcclusion;
	int			m_numAmbientOcclusionSamples;

	std::vector<AbstractLight *> m_lights;
	std::vector<const AbstractLight *> m_infiniteLights;
	int			m_lightCount;
	float		m_lightPickProbability;

	IImage *	m_frameBuffer;
	IImage *	m_normalBuffer;
	IImage *	m_depthBuffer;
	IImage *	m_albedoBuffer;
	IImage *	m_objectsBuffer;
	IImage *	m_materialsBuffer;
	const std::vector<IImage *> * m_layerBuffers;
	const ICameraProjector * m_projector;
	RenderArea	m_renderArea;
	int			m_iterationIndex;
	float		m_frameBlend;
	float		m_maxIntensity;
	float		m_photonScale;
	vec3		m_environmentIntensity;
	float		m_invGamma;
	vec2		m_dp, m_po;
	bool		m_dofEnabled;
	vec2		m_dofOffset[1024];
	vec2		m_bgScale;
	eBackgroundMode	m_bgMode;
	Floor		m_floor;

	std::atomic_size_t	m_rayCounter;
	std::atomic_int		m_tileCounter;
	int		m_numTilesX;
	int		m_numTiles;
	int		m_tileWidth;
	int		m_tileHeight;
	Random	m_random;

	float	m_rayLength;
	float	m_distEpsilon;
	uint	m_maxPathLength;
	uint	m_russianRouletteStart;
	int		m_numBumpLinearSteps;
	int		m_numBumpBinarySteps;

	QMutex	m_renderMutex;
//	QSemaphore		m_semaphore;
//	QThreadPool		m_threadPool;
//	std::atomic_int	m_taskCount;

	virtual void beginIteration() {}
	virtual void endIteration() {}

	const AbstractLight * getLight(size_t i) const { return i < m_lights.size() ? m_lights[i] : &m_environmentLight; }

	bool getNextTile(RectI & rc);
	void renderTile(const RectI & rc, Sampler & sampler);

	// Clear boundary stack and push in the global medium without any material
	void initBoundaryStack(BoundaryStack &oBoundaryStack) const
	{
		oBoundaryStack.Clear();
		oBoundaryStack.Push(StackElement(nullptr), GLOBAL_MEDIUM_PRIORITY); // Global medium has implicitly the lowest possible priority
	}

	bool handleVolumeSegment(Ray & oRay, Isect & oResult, VolumeSegments * oVolumeSegments, Sampler & sampler,
							 MaterialImpl * currentMediumPtr, float distMin, float distMax,
							 const float originStart, const float originEnd,
							 const uint32_t aRaySamplingFlags, const bool sampleMedia) const;

	static bool checkMaterialRefraction(MaterialImpl * material, const BoundaryStack & boundaryStack, bool backface);

	static void updateBoundaryStackOnRefract(const Isect & aIsect, BoundaryStack & oBoundaryStack);

	void intersect(Ray & ray) const
	{
		ray.resetHit();

		rtcIntersect1(m_rtcScene, &ray);
		++const_cast<std::atomic_size_t &>(m_rayCounter);
	}

	bool intersect(Ray & ray,
				   Isect & oResult,
				   Sampler & sampler,
				   BoundaryStack & oBoundaryStack,
				   const uint32_t aOptions,
				   const uint32_t aRaySamplingFlags,
				   VolumeSegments * oVolumeSegmentsToIsect) const;

	bool occluded(const vec3 & origin, const vec3 & direction, float distance,
				  const Ray & prevRay, Sampler & sampler,
				  const BoundaryStack & aBoundaryStack,
				  const uint32_t aRaySamplingFlags,
				  VolumeSegments & oVolumeSegments) const
	{
		Ray ray;
		ray.init(origin, direction, distance);
		if (prevRay.normalMap) {
			ray.normalMap = prevRay.normalMap;
			ray.dpdx = prevRay.dpdx;
			ray.dpdy = prevRay.dpdy;
			ray.dpdz = prevRay.dpdz;
			ray.bumpTC = prevRay.bumpTC;
			ray.bumpZ = prevRay.bumpZ;
			ray.bumpDepth = prevRay.bumpDepth;
			ray.hitBack = prevRay.hitBack;
		}

		Isect isect(distance);
		BoundaryStack stackCopy(aBoundaryStack);
		bool res = intersect(ray, isect, sampler, stackCopy, kOcclusionTest, aRaySamplingFlags, &oVolumeSegments);
		return res || isect.mFloor;
	}

	struct TraceResult {
		vec3  color;
		vec3  opacity;
		vec3  normal;
		float depth;
		vec3  albedo;
		const Node * node;
		const Geometry * geometry;

		TraceResult() : color(0.f), opacity(1.f), normal(0.f), depth(0.f), albedo(0.f), node(nullptr), geometry(nullptr) {}

		void addColor(const vec3 & c) { color += c; }
		vec3 objectColor() const { return node ? node->uniqueColor() : vec3(0.f); }
		vec3 materialColor() const { return geometry ? geometry->material()->uniqueColor() : vec3(0.f); }
		int renderLayer() const { return geometry ? geometry->meshNode()->renderLayer().getIndex() : 0; }
	};

	virtual void traceCameraPath(TraceResult & res, TraceContext & ctx) = 0;

	float evaluateShadowOpacity(TraceResult & res, TraceContext & ctx, const BoundaryStack & boundaryStack, VolumeSegments & volumeSegments);

	void prepareLights();
	void prepareDepthOfField(bool enable);

	void interrupt();

	virtual int getPreviewFramesCount() const { return 0; }

public:
	BaseRenderer(Scene & scene);
	virtual ~BaseRenderer();

	void restart(bool forceInterrupt);

	int frameCount() const { return m_frameCount; }
	RectI renderFrame(IImage & frameBuffer, IImage * albedoBuffer, IImage * normalBuffer);

	void runIteration(IImage & frameBuffer, IImage * albedoBuffer, IImage * normalBuffer, IImage * depthBuffer,
					  IImage * objectsBuffer, IImage * materialsBuffer, const std::vector<IImage *> * layerBuffers,
					  const RenderArea * renderArea, const vec2 & pixelOffset, int iterationIndex,
					  bool previewMode, float maxIntensity, float photonScale);

	void threadFunc();
};
