#include "BSDF.h"
#include "MaterialImpl.h"
#include "Microfacet.h"
#include "../PhaseFunction.h"

int	BSDF::g_frameIndex = 0;

// ------------------------------------------------------------------------ //

inline bool CheckLayerOpacity(MaterialLayerImpl *layer, const Ray & ray, Sampler & sampler)
{
	auto opacity = layer->mask().getScalar(ray);
	if (layer->diffuseLayer().get() && layer->diffuseTextureLayerMask().get() && layer->diffuseColor().texture()->isTransparent())
		opacity *= layer->diffuseColor().getOpacity(ray);
	return (opacity >= 1.f) || ((opacity > 0.f) && (opacity > sampler.generate1D()));
}

MaterialLayerImpl *BSDF::getNextLayer(const TraceContext & ctx) const
{
	auto layer = m_layer->getNext();
	while (layer) {
		if (CheckLayerOpacity(layer, ctx.ray, ctx.sampler))
			break;

		layer = layer->getNext();
	}

	return layer;
}

MaterialLayerImpl *BSDF::getPrevLayer(const TraceContext & ctx) const
{
	auto layer = m_layer->getPrev();
	while (layer) {
		if (CheckLayerOpacity(layer, ctx.ray, ctx.sampler))
			break;

		layer = layer->getPrev();
	}

	return layer;
}

MaterialGroupImpl * BSDF::getGroup(MaterialImpl * material, const Ray & ray, Sampler & sampler)
{
	for (const auto & group : material->groups()) {
		if (group->enabled().get() && !group->layers().empty()) {
			auto opacity = group->mask().getScalar(ray);
			if ((opacity >= 1.f) || ((opacity > 0.f) && (opacity > sampler.generate1D())))
				return group.get();
		}
	}

	return nullptr;
}

// ------------------------------------------------------------------------ //

MaterialLayerImpl * BSDF::findLayer(MaterialImpl * material, const Ray & ray, Sampler & sampler)
{
	if (ray.hitBack && !material->doubleSided().get() && material->isEmpty())
		return nullptr;

	assert(material);
	auto group = getGroup(material, ray, sampler);
	if (!group || group->layers().empty())
		return nullptr;

	if (ray.hitBack) { // exit
		auto layer = group->layers().back();
		while (layer) {
			if (CheckLayerOpacity(layer, ray, sampler))
				return layer;

			layer = layer->getPrev();
		}
	} else { // enter
		auto layer = group->layers().front();
		while (layer) {
			if (CheckLayerOpacity(layer, ray, sampler))
				return layer;

			layer = layer->getNext();
		}
	}

	return nullptr;
}

bool BSDF::simpleSetup(const TraceContext & ctx, MaterialImpl * material, bool isLightPath)
{
	assert(material);
	m_isLightPath = isLightPath;
	m_isOnSurface = true;
	m_material = material;
	m_layer = nullptr;
	m_dirFix = -ctx.ray.direction();
	m_iorExit = ctx.ray.hitBack ? nullptr : &m_material->indexOfRefraction();

	auto group = getGroup(material, ctx.ray, ctx.sampler);
	m_emission = vec3(0.f);// (!ctx.ray.hitBack && group) ? group->Emission().GetColor(ctx.ray) * group->EmissionIntensity().value() : vec3(0.f);

	if (group && !group->layers().empty()) { // setup layer if such available
		if (ctx.ray.hitBack) {
			auto layer = group->layers().back();
			while (layer) {
				if (CheckLayerOpacity(layer, ctx.ray, ctx.sampler))
					return setupLayer(ctx, layer, 0, ctx.ior);

				layer = layer->getPrev();
			}
		} else {
			auto layer = group->layers().front();
			while (layer) {
				if (CheckLayerOpacity(layer, ctx.ray, ctx.sampler))
					return setupLayer(ctx, layer, 0, ctx.ior);

				layer = layer->getNext();
			}
		}
	}

	if (m_material->isOpaque() || getIOR(ctx.ior) != getIOR(m_iorExit))
		return setupExitLayer(ctx, nullptr);

	return false;
}

bool BSDF::setupFloor(const TraceContext & ctx, const Isect & isect, float reflectionLevel, const Floor & floor, bool isLightPath)
{
	assert(isect.mFloor);
	m_isLightPath = isLightPath;
	m_isOnSurface = isect.isOnSurface();
	m_dirFix = -ctx.ray.direction();

	m_isSpecular = reflectionLevel >= 1.f || reflectionLevel > ctx.sampler.generate1D();
	m_material = nullptr;
	m_emission = vec3(0.f);

	m_normal = m_geomNormal = vec3(0.f, 0.f, 1.f);
	m_frame.setFromZ(vec3(0.f, 0.f, 1.f));

	if (m_isSpecular) {
		m_reflection = vec3(1.f);
		m_reflectionProbability = 1.f;

		vec3 wo = m_frame.toLocal(m_dirFix);
		wo.z = std::max(wo.z, EPS_COSINE);
		BeckmannDistribution microfacetDistribution(vec2(floor.roughness), true);
		for (int i = 0; i < 3; i++) {
			auto uSample = ctx.sampler.generate2D();
			auto n = microfacetDistribution.sample_wh(wo, uSample);
			if (!isfinite(n.x) || !isfinite(n.y) || !isfinite(n.z)) {
				LogInformation() << QString("Beckmann distribution error: n(%1,%2,%3) wo(%4,%5,%6) s=(%7,%8)").arg(n.x).arg(n.y).arg(n.z).arg(wo.x).arg(wo.y).arg(wo.z).arg(uSample.x).arg(uSample.y);
				continue;
			}
			assert(isfinite(n.x) && isfinite(n.y) && isfinite(n.z));
			m_normal = m_frame.toWorld(n);
			auto wi = glm::reflect(ctx.ray.direction(), m_normal);
			if (glm::dot(wi, m_geomNormal) > EPS_COSINE)
				break;
		}
	} else {
		m_reflection = floor.diffuseIntensity;
		m_diffuseProbability = 1.f;
		m_opacity = 1.f;
		m_sigma = 0.f;
	}

	m_cosI = glm::dot(m_dirFix, m_normal);
	m_cosI = std::clamp(m_cosI, EPS_COSINE, 1.f);
	m_cosThetaFix = std::fabs(glm::dot(m_dirFix, m_geomNormal));
	m_cosThetaFix = std::max(m_cosThetaFix, EPS_COSINE);

	m_transmission = vec3(0.f);
	m_transmissionProbability = 0.f;
	m_continuationProbability = 1.f;
	return true;
}

bool BSDF::setup(const TraceContext & ctx, const Isect & isect, const BoundaryStack & boundaryStack, bool isLightPath)
{
	m_isLightPath = isLightPath;
	m_isOnSurface = isect.isOnSurface();
	m_dirFix = -ctx.ray.direction();

	if (!m_isOnSurface)
		return setupMedium(ctx, isect.mMedium); // medium

	if (isect.mFloor) {
		m_isSpecular = true;
		m_material = nullptr;

		m_normal = m_geomNormal = vec3(0.f, 0.f, 1.f);
		m_frame.setFromZ(vec3(0.f, 0.f, 1.f));

		vec3 wo = m_frame.toLocal(m_dirFix);
		wo.z = std::max(wo.z, EPS_COSINE);
		BeckmannDistribution microfacetDistribution(vec2(0.1f), true);
		for (int i = 0; i < 3; i++) {
			auto uSample = ctx.sampler.generate2D();
			auto n = microfacetDistribution.sample_wh(wo, uSample);
			if (!isfinite(n.x) || !isfinite(n.y) || !isfinite(n.z)) {
				LogInformation() << QString("Beckmann distribution error: n(%1,%2,%3) wo(%4,%5,%6) s=(%7,%8)").arg(n.x).arg(n.y).arg(n.z).arg(wo.x).arg(wo.y).arg(wo.z).arg(uSample.x).arg(uSample.y);
				continue;
			}
			assert(isfinite(n.x) && isfinite(n.y) && isfinite(n.z));
			m_normal = m_frame.toWorld(n);
			auto wi = glm::reflect(ctx.ray.direction(), m_normal);
			if (glm::dot(wi, m_geomNormal) > EPS_COSINE)
				break;
		}

		m_cosI = glm::dot(m_dirFix, m_normal);
		m_cosI = std::clamp(m_cosI, EPS_COSINE, 1.f);
		m_cosThetaFix = std::fabs(glm::dot(m_dirFix, m_geomNormal));
		m_cosThetaFix = std::max(m_cosThetaFix, EPS_COSINE);

		m_reflection = vec3(1.f);
		m_transmission = vec3(0.f);
		m_reflectionProbability = 1.f;
		m_transmissionProbability = 0.f;
		m_continuationProbability = 1.f;
		return true;
	}

	if (isect.mLight) {
		m_isSpecular = true;
		m_material = nullptr;

		m_normal = m_geomNormal = ctx.ray.hitBack ? -ctx.ray.geomNormal : ctx.ray.geomNormal;
		m_frame.setFromZ(m_normal);

		m_cosI = glm::dot(m_dirFix, m_normal);
		m_cosI = std::clamp(m_cosI, EPS_COSINE, 1.f);
		m_cosThetaFix = std::fabs(glm::dot(m_dirFix, m_geomNormal));
		m_cosThetaFix = std::max(m_cosThetaFix, EPS_COSINE);

		m_continuationProbability = 0.f;
		return true;
	}

	m_material = isect.mGeometry->material();
	assert(m_material);

	auto layer = isect.mMaterial;
	if (isect.mEnter) { // enter
		m_iorExit = !m_material->isEmpty() ? &m_material->indexOfRefraction() : ctx.ior;

		if (layer) {
			//auto group = layer->GetGroup();
			m_emission = vec3(0.f);// group ? group->Emission().GetColor(ctx.ray) * group->EmissionIntensity().value() : vec3(0.f);

			return setupLayer(ctx, layer, 0, ctx.ior); // enter surface layer
		} else if (!m_material->isOpaque()) {
			m_emission = vec3(0.f);
			return setupExitLayer(ctx, nullptr);
		} else
			return false;
	} else {// exit
		m_iorExit = boundaryStack.Top().mMedium != m_material ? boundaryStack.Top().mIOR : boundaryStack.SecondFromTop().mIOR;

		m_emission = vec3(0.f);
		return layer ? setupLayer(ctx, layer, 0, ctx.ior) : setupExitLayer(ctx, nullptr); // exit surface layer
	}
}

bool BSDF::setupExitLayer(const TraceContext & ctx, MaterialLayerImpl * prevLayer)
{
	m_layer = nullptr;
	m_isSpecular = true;

	if (prevLayer) {

		float cosT;
		auto n1 = getIOR(ctx.ior).real();
		auto n2 = getIOR(&prevLayer->indexOfRefraction()).real();
		if (n1 != n2) {
			auto eta = n1 / n2;
			auto sinI2 = 1.f - m_cosI * m_cosI;
			auto cosT2 = 1.f - sqr(eta) * sinI2;
			cosT = std::sqrt(std::max(cosT2, 0.f));
		} else
			cosT = m_cosI;

		auto reflectance = getFresnelReflectance(cosT, &prevLayer->indexOfRefraction(), m_iorExit, g_frameIndex);
		auto reflectCoeff = luminance(reflectance);
		updateReflection90Tint(prevLayer, ctx);
		m_reflection *= reflectance;
		m_transmission = vec3(1.f - reflectCoeff);
	} else {
		m_geomNormal = ctx.ray.hitBack ? -ctx.ray.geomNormal : ctx.ray.geomNormal;
		m_normal = ctx.ray.hitBack ? -ctx.ray.normal : ctx.ray.normal;
		m_frame.setFromZ(m_normal);

		m_cosI = glm::dot(m_dirFix, m_normal);
		m_cosI = std::clamp(m_cosI, EPS_COSINE, 1.f);
		m_cosThetaFix = std::fabs(glm::dot(m_dirFix, m_geomNormal));
		m_cosThetaFix = std::max(m_cosThetaFix, EPS_COSINE);

		m_reflection = vec3(0.f);
		m_transmission = m_material->isOpaque() ? vec3(0.f) : vec3(1.f);

		m_reflectionProbability = 0.f;
		m_transmissionProbability = 1.f;
		m_continuationProbability = 1.f;
	}

	return true;
}

bool BSDF::setupLayer(const TraceContext & ctx, MaterialLayerImpl * layer, int subLayer, const IndexOfRefractionImpl * iorPrev)
{
	assert(layer);
	m_layer = layer;
	m_isSpecular = subLayer == 0 && m_layer->isSpecular();

	setupNormal(ctx, m_layer);

	m_cosI = glm::dot(m_dirFix, m_normal);
	m_cosI = std::clamp(m_cosI, EPS_COSINE, 1.f);
	m_cosThetaFix = std::fabs(glm::dot(m_dirFix, m_geomNormal));
	m_cosThetaFix = std::max(m_cosThetaFix, EPS_COSINE);

	vec3 emission = subLayer == 0 && m_layer->emissiveLayer().get() && !ctx.ray.hitBack ? m_layer->emissiveColor().getColor(ctx.ray) * m_layer->emissiveIntensity().rgb() : vec3(0.f);

	if (m_isSpecular) {
		m_reflection = m_layer->reflection().isEnabled() ? m_layer->reflection().getColor(ctx.ray) : vec3(1.f);

		MaterialLayerImpl *nextLayer;
		if (subLayer == 0 && m_layer->diffuseLayer().get()) {
			nextLayer = m_layer;
			subLayer++;
		} else
			nextLayer = ctx.ray.hitBack ? getPrevLayer(ctx) : getNextLayer(ctx);

		vec3 reflectance;
		float reflectCoeff;

		auto * iorNext = &m_layer->indexOfRefraction();
		if (m_layer && m_layer->thinFilmInterference().get()) {
			auto thickness = m_layer->thickness().value();
			if (m_layer->thickness().texture()->valid())
				thickness = lerp(m_layer->minThickness().value(), thickness, m_layer->thickness().texture()->getScalar(ctx.ray));

			reflectance = getThinFilmReflectance(reflectCoeff, m_cosI, thickness, g_frameIndex,
												 iorPrev, &m_layer->filmIndexOfRefraction(), iorNext);
		} else {
			reflectance = getFresnelReflectance(m_cosI, iorPrev, iorNext, g_frameIndex);
			reflectCoeff = luminance(reflectance);
		}

		if ((m_material->isOpaque() && !nextLayer) ||
			(m_layer->indexOfRefraction().iorType() == IORType::MEASURED && m_layer->indexOfRefraction().attenuation() == 0.f))
			m_transmission = vec3(0.f); // opaque
		else if (m_layer->transmission().isEnabled())
			m_transmission = m_layer->transmission().getColor(ctx.ray);
		else // measured
			m_transmission = vec3(1.f);

		float transmittance = 1.f - reflectCoeff;
		if (!isBlack(m_transmission) && transmittance > 0.f) {
			if (reflectCoeff == 0.f || reflectCoeff < ctx.sampler.generate1D()) {
				auto transmission = m_transmission;
				m_emission += emission * m_transmission;
				auto res = nextLayer ? setupLayer(ctx, nextLayer, subLayer, iorNext) : setupExitLayer(ctx, m_layer);
				m_reflection *= transmission;
				m_transmission *= transmission;
				if (m_isSpecular)
					getSpecularComponentProbabilities();
				else
					m_continuationProbability = maxComponent(m_reflection);
				return res;
			}

			reflectance /= reflectCoeff;
			transmittance = 0.f;
		}

		updateReflection90Tint(m_layer, ctx);

		m_reflection *= reflectance;
		m_transmission *= transmittance;
		emission *= m_transmission;

		getSpecularComponentProbabilities();
	} else if (m_layer->diffuseLayer().get() &&
			 (m_layer->diffuseTextureLayerMask().get() || !m_layer->diffuseColor().texture()->isTransparent() || m_layer->diffuseColor().getOpacity(ctx.ray) >= ctx.sampler.generate1D())) { // diffuse
		m_reflection = m_layer->diffuseColor().getColor(ctx.ray) * m_material->owner().scene().diffuseIntensity().rgb();
		m_opacity = m_layer->diffuseOpacity().getScalar(ctx.ray);
		m_transmission = m_opacity < 1.f && !m_material->isOpaque() ? m_layer->diffuseTransmission().getColor(ctx.ray) : vec3(0.f);
		m_sigma = 0;//m_layer->Roughness().GetScalar(ctx.ray) * M_PI_4f;
		m_diffuseProbability = 1.f;
		m_continuationProbability = maxComponent(m_reflection);
	} else {
//		assert(layer->EmissiveLayer().get() && !layer->SpecularLayer().get() && !layer->DiffuseLayer().get()); // emissive layer only
		m_emission += emission;
		auto nextLayer = ctx.ray.hitBack ? getPrevLayer(ctx) : getNextLayer(ctx);
		return nextLayer ? setupLayer(ctx, nextLayer, 0, iorPrev) : setupExitLayer(ctx, nullptr);
	}

	m_emission += emission;

	return true;
}

bool BSDF::setupMedium(const TraceContext & ctx, MaterialImpl * material)
{
	if (!material) return false;

	m_isSpecular = false;
	m_material = material;
	m_layer = nullptr;
	m_emission = vec3(0.f);

	m_normal = m_geomNormal = ctx.ray.direction();
	m_frame.setFromZ(m_normal);

	m_cosThetaFix = 1.f;
	m_cosI = 1.f;

	m_continuationProbability = m_material->continuationProbability();
	m_meanCosine = std::clamp(m_material->meanCosine(), -1.f + 1e-4f, 1.f - 1e-6f);
	m_scatterCoef = m_material->getScatteringCoef();
	m_scatterCoefPos = isBlack(m_scatterCoef) ? 0 : 1;
	return true;
}

// ------------------------------------------------------------------------ //

void BSDF::updateReflection90Tint(MaterialLayerImpl * layer, const TraceContext & ctx)
{
	if (!layer->reflection90().isEnabled())
		return;

	auto refl90Level = layer->reflection90Level().value();
	if (refl90Level > 0.f) {
		auto reflection90 = layer->reflection90().getColor(ctx.ray);
		if (refl90Level < 1.f) {
			auto angle = std::acos(m_cosI) * (2.f * M_1_PIf);
			if (refl90Level <= 0.5f)
				angle = std::pow(angle, 0.5f / refl90Level);
			else
				angle = 1.f - std::pow(1.f - angle, 0.5f / (1.f - refl90Level));

			m_reflection = glm::mix(m_reflection, reflection90, angle);
		} else
			m_reflection = reflection90;
	}
}

void BSDF::getSpecularComponentProbabilities()
{
	auto albedoReflection = luminance(m_reflection);
	auto albedoTransmission = luminance(m_transmission);
	auto totalAlbedo = albedoReflection + albedoTransmission;
	m_reflectionProbability = albedoReflection / totalAlbedo;
	m_transmissionProbability = 1.f - m_reflectionProbability;

	m_continuationProbability = maxComponent(m_reflection) + maxComponent(m_transmission);
}

void BSDF::setupNormal(const TraceContext &ctx, MaterialLayerImpl * layer)
{
	m_geomNormal = ctx.ray.hitBack ? -ctx.ray.geomNormal : ctx.ray.geomNormal;

	if (layer->hasBump()) {
		auto normalMap = layer->bump().texture();
		vec3 dpdx, dpdy;
		normalMap->getTangentBinormal(dpdx, dpdy, ctx.ray, layer->bump().value());
		m_normal = getBumpNormal(dpdx, dpdy, ctx.ray.normal, normalMap, normalMap->getTC(ctx.ray));
		if (ctx.ray.hitBack)
			m_normal = -m_normal;
	} else
		m_normal = ctx.ray.hitBack ? -ctx.ray.normal : ctx.ray.normal;

	m_frame.setFromZ(m_normal);

	if (!m_isSpecular)
		return;

	auto roughness = layer->roughness().getScalar(ctx.ray);
	auto anisotropy = layer->anisotropy().getScalar(ctx.ray);
	if (roughness > 0.f || anisotropy > 0.f) {
		m_alpha = vec2(std::max(roughness, 1e-3f));

		if (anisotropy > 0.f) {
			m_alpha.y = std::min(m_alpha.y + anisotropy, 1.f);

			m_frame.mX = layer->anisotropy().texture()->getTangentDir(ctx.ray);
			auto angle = layer->anisotropyAngle().get();
			if (layer->anisotropyAngle().texture()->valid())
				angle += layer->anisotropyAngle().texture()->getScalar(ctx.ray) * 360.f;
			if (angle > 0.f)
				m_frame.mX = glm::rotate(m_frame.mX, -glm::radians(angle), m_frame.mZ);

			m_frame.mY = glm::cross(m_frame.mZ, m_frame.mX);
		}

		vec3 dir = -ctx.ray.direction();
		vec3 wo = m_frame.toLocal(dir);
		wo.z = std::max(wo.z, EPS_COSINE);
		BeckmannDistribution microfacetDistribution(m_alpha, true);
//		TrowbridgeReitzDistribution microfacetDistribution(m_alpha, true);
		for (int i = 0; i < 3; i++) {
			auto uSample = ctx.sampler.generate2D();
			auto n = microfacetDistribution.sample_wh(wo, uSample);
			if (!isfinite(n.x) || !isfinite(n.y) || !isfinite(n.z)) {
				LogInformation() << QString("Beckmann distribution error: n(%1,%2,%3) wo(%4,%5,%6) s=(%7,%8)").arg(n.x).arg(n.y).arg(n.z).arg(wo.x).arg(wo.y).arg(wo.z).arg(uSample.x).arg(uSample.y);
				continue;
			}
			assert(isfinite(n.x) && isfinite(n.y) && isfinite(n.z));
			m_normal = m_frame.toWorld(n);
			auto wi = glm::reflect(ctx.ray.direction(), m_normal);
			if (glm::dot(wi, m_geomNormal) > EPS_COSINE)
				break;
		}
	}
}

// ------------------------------------------------------------------------ //

float BSDF::orenNayarFactor(const vec3 &wo, const vec3 &wi) const
{
	float sinThetaI = sinTheta(wi);
	float sinThetaO = sinTheta(wo);
	// Compute cosine term of Oren-Nayar model
	float maxCos = 0;
	if (sinThetaI > 1e-4f && sinThetaO > 1e-4f) {
		float sinPhiI = sinPhi(wi), cosPhiI = cosPhi(wi);
		float sinPhiO = sinPhi(wo), cosPhiO = cosPhi(wo);
		float dCos = cosPhiI * cosPhiO + sinPhiI * sinPhiO;
		maxCos = std::max(0.f, dCos);
	}

	// Compute sine and tangent terms of Oren-Nayar model
	float sinAlpha, tanBeta;
	if (abscosTheta(wi) > abscosTheta(wo)) {
		sinAlpha = sinThetaO;
		tanBeta = sinThetaI / abscosTheta(wi);
	} else {
		sinAlpha = sinThetaI;
		tanBeta = sinThetaO / abscosTheta(wo);
	}

	float sigma2 = m_sigma * m_sigma;
	float A = 1.f - 0.5f * sigma2 / (sigma2 + 0.33f);
	float B = 0.45f * sigma2 / (sigma2 + 0.09f);

	return A + B * maxCos * sinAlpha * tanBeta;
}

vec3 BSDF::evaluate(const vec3 & aWorldDirGen, // Points away from the scattering location
					float & oCosThetaGen,
					float * oDirectPdfW,
					float * oReversePdfW,
					float * oSinTheta) const
{
	if (m_isOnSurface) { // surface
		if (oDirectPdfW)  *oDirectPdfW = 0;
		if (oReversePdfW) *oReversePdfW = 0;
		if (oSinTheta)    *oSinTheta = 0;
		if (m_cosThetaFix < EPS_COSINE)
			return vec3(0.f);

		const float cosThetaGen = glm::dot(aWorldDirGen, m_normal);

		oCosThetaGen = std::fabs(cosThetaGen);
		if (oCosThetaGen < EPS_COSINE)
			return vec3(0.f);

		float f = cosThetaGen >= 0.f ? m_opacity : (1.f - m_opacity);
		if (f <= 0.f)
			return vec3(0.f);

		if (oDirectPdfW)
			*oDirectPdfW += m_diffuseProbability * std::max(0.f, oCosThetaGen * M_1_PIf);

		if (oReversePdfW)
			*oReversePdfW += m_diffuseProbability * std::max(0.f, m_cosThetaFix * M_1_PIf);

		if (m_sigma > 0.f) { // Oren�Nayar reflectance
			auto localDirGen = m_frame.toLocal(aWorldDirGen);
			if (cosThetaGen < 0.f) localDirGen.z = -localDirGen.z;
			f *= orenNayarFactor(m_frame.toLocal(m_dirFix), localDirGen);
		}

		auto factor = m_reflection * (M_1_PIf * f);
		if (cosThetaGen < 0.f)
			factor *= m_transmission;

		return factor;
	} else // medium
	{
		oCosThetaGen = 1.0f;

		// No need to evaluate phase function if the scattering coef is zero
		if (!m_scatterCoefPos) {
			if (oDirectPdfW)  *oDirectPdfW = 0;
			if (oReversePdfW) *oReversePdfW = 0;
			if (oSinTheta)    *oSinTheta = 0;
			return vec3(0.f);
		}

		return m_scatterCoef * PhaseFunction::evaluate(m_dirFix, aWorldDirGen, m_meanCosine, oDirectPdfW, oReversePdfW, oSinTheta);
	}
}

float BSDF::pdf(const vec3 & aWorldDirGen, float cosL, bool aEvalRevPdf) const
{
	if (isOnSurface()) { // surface
		assert(!m_isSpecular);

		if (m_cosThetaFix * cosL < 0)
			return 0.f;

		return m_diffuseProbability * M_1_PIf * (aEvalRevPdf ? m_cosThetaFix : cosL);
	} else // medium
	{
		return PhaseFunction::pdf(m_dirFix, aWorldDirGen, m_meanCosine);
	}
}

vec3 BSDF::sample(TraceContext & ctx,
				  const vec3 & sample,
				  vec3 & wi, // incoming light direction
				  float & pdfW, float & cosThetaOut,
				  uint32_t * oSampledEvent,
				  float * oSinTheta) const
{
	uint32_t sampledEvent;
	vec3 factor;
	if (m_isOnSurface) { // surface
		if (m_isSpecular) {
			if (m_reflectionProbability >= sample.z && m_reflectionProbability > 0.f) { // relfection
				sampledEvent = kReflect;
				ctx.floor = !m_material;

				wi = glm::normalize(glm::reflect(ctx.ray.direction(), m_normal));

				cosThetaOut = m_cosThetaFix;

				pdfW = m_reflectionProbability;

				// BSDF is multiplied (outside) by cosine (oLocalDirGen.z),
				// for mirror this shouldn't be done, so we pre-divide here instead
				factor = m_reflection / std::fabs(m_cosThetaFix);
			} else {// refraction
				sampledEvent = kRefract;

				auto n1 = getIOR(ctx.ior).real();
				auto n2 = getIOR(m_iorExit).real();
				auto eta = n1 / n2;

				assert(m_material);
				if (!m_material->isEmpty() && n1 != n2)
					ctx.floor = false;

				ctx.ior = m_iorExit;

				auto sinI2 = 1.f - m_cosI * m_cosI;
				auto cosT2 = 1.f - sqr(eta) * sinI2;
				auto cosT = std::sqrt(std::max(cosT2, EPS_COSINE * EPS_COSINE));
				cosThetaOut = cosT;

				wi = glm::normalize(eta * ctx.ray.direction() + (eta * m_cosI - cosT) * m_normal);

				pdfW = m_transmissionProbability;

				factor = m_transmission;

				// only camera paths are multiplied by this factor, and etas
				// are swapped because radiance flows in the opposite direction
				if (m_isLightPath)
					factor *= sqr(eta) / std::fabs(cosT);
				else
					factor /= std::fabs(cosT);
			}
		} else {// diffuse reflection
			if (sample.z > m_opacity) { // tranmission
				sampledEvent = kTransmit;
				ctx.ior = m_iorExit;
			} else
				sampledEvent = kDiffuse;

			ctx.floor = false;
			auto localDirGen = cosineSampleHemisphere((const vec2 &)sample, &pdfW);

//			pdfW *= m_diffuseProbability;
			auto n = getIOR(ctx.ior).real();
			localDirGen.z *= n;
			wi = glm::normalize(m_frame.toWorld(sampledEvent == kTransmit ? vec3(localDirGen.x, localDirGen.y, -localDirGen.z) : localDirGen));

			cosThetaOut = std::max(localDirGen.z, EPS_COSINE);

			float f = m_sigma > 0.f ? orenNayarFactor(m_frame.toLocal(m_dirFix), localDirGen) : 1.f;

			factor = (sampledEvent == kTransmit ? m_transmission : m_reflection) * (M_1_PIf * f);
		}

		auto dp = glm::dot(wi, m_geomNormal);
		if (sampledEvent & (kReflect | kDiffuse | kPhong)) { // reflected
			if (m_material != nullptr) // if not floor
				ctx.reflected = true;

			if (dp < EPS_COSINE) {
				wi += m_geomNormal * (EPS_COSINE - dp);
				wi = glm::normalize(wi);
			}
		} else {// transmitted
			if (dp > -EPS_COSINE) {
				wi -= m_geomNormal * (EPS_COSINE + dp);
				wi = glm::normalize(wi);
			}
		}

		if (oSinTheta) *oSinTheta = 0.f;

		assert(isRoughlyNormalized(wi));
	} else {// medium
		cosThetaOut = 1.0f;
		sampledEvent = kScatter;
		ctx.reflected = true;

		// No need to evaluate phase function if the scattering coef is zero
		if (!m_scatterCoefPos) {
			assert(false);
			wi = vec3(0.f);
			pdfW = 0;
			if (oSinTheta) *oSinTheta = 0.f;
			factor = vec3(0.f);
		} else
			factor = m_scatterCoef * PhaseFunction::sample(m_dirFix, m_meanCosine, sample, m_frame, wi, pdfW, oSinTheta);
	}

	if (oSampledEvent)
		*oSampledEvent = sampledEvent;

	return factor;
}
