#pragma once

struct Ray;
struct TraceContext;
class MaterialImpl;
class MaterialGroupImpl;
class MaterialLayerImpl;
class IndexOfRefractionImpl;

class BSDF
{
	MaterialLayerImpl * m_layer;
	MaterialImpl * m_material;
	const IndexOfRefractionImpl * m_iorExit;
	bool	m_isOnSurface;
	bool	m_isSpecular;
	bool	m_isLightPath;

	vec3	m_reflection;
	vec3	m_transmission;
	vec3	m_emission;
	Frame	m_frame;
	vec2	m_alpha; // roughness
	vec3	m_normal;
	vec3	m_geomNormal;
//	vec3	m_wo; // outgoing viewer direction
//	vec3	m_wi; // incoming light direction
	float	m_cosI; // outgoing direction
	float	m_cosThetaFix;
	float	m_sigma;
	float	m_opacity; // diffuse opacity
	vec3	m_dirFix;

	float	m_diffuseProbability;
	float	m_reflectionProbability;
	float	m_transmissionProbability;
	float	m_continuationProbability;

	float	m_meanCosine;
	vec3	m_scatterCoef;
	int		m_scatterCoefPos;

	MaterialLayerImpl *getNextLayer(const TraceContext & ctx) const;
	MaterialLayerImpl *getPrevLayer(const TraceContext & ctx) const;

	void setupNormal(const TraceContext & ctx, MaterialLayerImpl * layer);

	void getSpecularComponentProbabilities();
	void updateReflection90Tint(MaterialLayerImpl * layer, const TraceContext & ctx);
	float orenNayarFactor(const vec3 &wo, const vec3 &wi) const;

	bool setupLayer(const TraceContext & ctx, MaterialLayerImpl * layer, int subLayer, const IndexOfRefractionImpl * iorPrev);
	bool setupExitLayer(const TraceContext & ctx, MaterialLayerImpl * prevLayer);
	bool setupMedium(const TraceContext & ctx, MaterialImpl * material);

	static MaterialGroupImpl * getGroup(MaterialImpl * material, const Ray & ray, Sampler & sampler);

public:
	static int	g_frameIndex;

	static MaterialLayerImpl * findLayer(MaterialImpl * material, const Ray & ray, Sampler & sampler);

	bool simpleSetup(const TraceContext & ctx, MaterialImpl * material, bool isLightPath);
	bool setup(const TraceContext & ctx, const Isect & isect, const BoundaryStack & boundaryStack, bool isLightPath);
	bool setupFloor(const TraceContext & ctx, const Isect & isect, float reflectionLevel, const Floor & floor, bool isLightPath);

	bool isOnSurface() const { return m_isOnSurface; }
	bool isInMedium() const { return !m_isOnSurface; }
	bool isDelta() const { return m_isSpecular; }
	bool isSpecular() const { return m_isSpecular; }
	bool isFromCamera() const { return !m_isLightPath; }
	bool isFromLight() const { return m_isLightPath; }
	float cosThetaFix() const { return m_cosThetaFix; }
	const vec3 & worldDirFix() const { return m_dirFix; }
	const MaterialImpl* material() const { return m_isOnSurface ? m_material : nullptr; }
	const MaterialImpl* medium() const { return m_isOnSurface ? nullptr : m_material; }
	float continuationProbability() const { return m_continuationProbability; }
	const vec3 & emission() const { return m_emission; }
	vec3 albedo() const { return m_reflection + m_emission; }
	const vec3 & normal() const { return m_normal; }
	const vec3 & geomNormal() const { return m_geomNormal; }

	// Given a direction, evaluates BSDF
	// Returns value of BSDF, as well as cosine for the
	// aWorldDirGen direction.
	// Can return probability (w.r.t. solid angle W),
	// of having sampled aWorldDirGen given mLocalDirFix (oDirectPdfW),
	// and of having sampled mLocalDirFix given aWorldDirGen (oReversePdfW).
	vec3 evaluate(const vec3 & aWorldDirGen, // Points away from the scattering location
				  float & oCosThetaGen,
				  float * oDirectPdfW = nullptr,
				  float * oReversePdfW = nullptr,
				  float * oSinTheta = nullptr) const;

	float pdf(const vec3 & aWorldDirGen, float cosL, bool aEvalRevPdf) const;

	enum Events {
		kNONE        = 0,
		kDiffuse     = 1,
		kPhong       = 2,
		kReflect     = 4,
		kRefract     = 8,
		kTransmit    = 16,
		kScatter     = 32, // media
		kSpecular    = (kReflect  | kRefract),
		kNonSpecular = (kDiffuse  | kPhong | kScatter),
		kAll         = (kSpecular | kNonSpecular)
	};

	vec3 sample(TraceContext & ctx, const vec3 & random,
				vec3 & wi, // incoming light direction
				float & pdfW, float & cosThetaOut,
				uint32_t * oSampledEvent = nullptr,
				float * oSinTheta = nullptr) const;
};
