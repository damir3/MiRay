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

#include "RenderMath.h"
#include "Materials/IndexOfRefractionImpl.h"
#include "TextureParametersImpl.h"

vec3 getFresnelReflectance(float cosI, const IndexOfRefractionImpl * ior1, const IndexOfRefractionImpl * ior2, int offset)
{
	if (compare(ior1, ior2))
		return vec3(0.f);

	if ((ior1 && ior1->isDynamic()) || (ior2 && ior2->isDynamic())) {
		auto & spectrumColors = IndexOfRefractionImpl::spectrumColors();
		vec3 xyzReflectance(0.f);
		int count = 0;
		if (offset < 0) {
			for (size_t i = 0; i < spectrumColors.size(); ++i, ++count) {
				auto & sc = spectrumColors[i];
				auto refl = fresnelReflectance(getIOR(ior1, sc.waveLength), getIOR(ior2, sc.waveLength), cosI);
				xyzReflectance += refl * sc.xyz;
			}
		} else {
			for (size_t i = offset % 8; i < spectrumColors.size(); i += 8, count++) {
				auto & sc = spectrumColors[i];
				auto refl = fresnelReflectance(getIOR(ior1, sc.waveLength), getIOR(ior2, sc.waveLength), cosI);
				xyzReflectance += refl * sc.xyz;
			}
		}
		return xyzToRGB(xyzReflectance / (float)count);
	}

	return vec3(fresnelReflectance(getIOR(ior1), getIOR(ior2), cosI));
}

vec3 getThinFilmReflectance(float & averageReflectance, float cosI, float thickness, int offset,
							const IndexOfRefractionImpl * ior0, const IndexOfRefractionImpl * ior1, const IndexOfRefractionImpl * ior2)
{
	auto & spectrumColors = IndexOfRefractionImpl::spectrumColors();
	auto sinI = std::sqrt(1.f - cosI * cosI);
	vec3 xyzReflectance(0.f);
	averageReflectance = 0.f;
	int c = 0;
	for (size_t i = offset % 8; i < spectrumColors.size(); i += 8, c++) {
		auto & sc = spectrumColors[i];
		auto iorOuter = getIOR(ior0, sc.waveLength);
		auto iorFilm = ior1->getIOR(sc.waveLength);
		auto iorLayer = getIOR(ior2, sc.waveLength);

		auto refl = getThinFilmReflectance(iorOuter, iorFilm, iorLayer, thickness, cosI, sc.waveLength);

		averageReflectance += refl;
		xyzReflectance += sc.xyz * refl;
	}

	float f = 1.f / (float)c;
	averageReflectance *= f;
//	reflectance *= IndexOfRefractionImpl::GetSpectrumColorScale() * f;
	xyzReflectance *= f;
	return xyzToRGB(xyzReflectance);
}

float getThinFilmReflectance(const Complex & nO, const Complex & nF, const Complex & nM, float thickness, float cosI, float waveLength)
{
	if (nO == nF && nF == nM)
		return 0.f; // returns 0 first

	if (cosI <= 0.f)
		return 1.f; // total internal reflection at zero grazing angle

	// handle edge/trivial cases: if film is nonexistent or wavelength is invalid, fall back to
	// standard Fresnel reflection at the boundary between outer medium and substrate
	if (thickness <= 0.f || waveLength <= 0.f)
		return fresnelReflectance(nO, nM, cosI);

	cosI = std::min(cosI, 1.f); // clamp to avoid numerical issues with sqrt(1 - cosI^2)
	float sinO = std::sqrt(1.f - cosI * cosI); // sin of the incident angle in the outer medium (O)

	// apply Snell's Law for complex indices of refraction to calculate complex angles of refraction:
	// film layer (F):
	Complex sinF = (nO * sinO) / nF; // sin of the refracted angle in the thin film (F)
	Complex cosF = std::sqrt(Complex(1.f, 0.f) - sinF * sinF); // cos of the refracted angle in the thin film (F)

	// substrate material layer (M):
	Complex sinM = (nO * sinO) / nM; // sin of the refracted angle in the substrate layer (M)
	Complex cosM = std::sqrt(Complex(1.f, 0.f) - sinM * sinM); // cos of the refracted angle in the substrate layer (M)

	// pre-calculate products to avoid duplicate complex multiplications
	Complex nO_cosO = nO * cosI;
	Complex nF_cosI = nF * cosI;
	Complex nF_cosF = nF * cosF;
	Complex nO_cosF = nO * cosF;
	Complex nM_cosM = nM * cosM;
	Complex nM_cosF = nM * cosF;
	Complex nF_cosM = nF * cosM;

	// calculate Fresnel reflection coefficients at interface 1 (Outer medium -> Thin Film):
	// s-polarization (TE): rs = (n_i * cos_i - n_t * cos_t) / (n_i * cos_i + n_t * cos_t)
	Complex rs_OF_den = nO_cosO + nF_cosF;
	Complex rs_OF = rs_OF_den != 0.f ? (nO_cosO - nF_cosF) / rs_OF_den : 0.f;

	// p-polarization (TM): rp = (n_t * cos_i - n_i * cos_t) / (n_t * cos_i + n_i * cos_t)
	Complex rp_OF_den = nF_cosI + nO_cosF;
	Complex rp_OF = rp_OF_den != 0.f ? (nF_cosI - nO_cosF) / rp_OF_den : 0.f;

	// calculate Fresnel reflection coefficients at Interface 2 (Thin Film -> Substrate):
	// s-polarization (TE):
	Complex rs_FM_den = nF_cosF + nM_cosM;
	Complex rs_FM = rs_FM_den != 0.f ? (nF_cosF - nM_cosM) / rs_FM_den : 0.f;

	// p-polarization (TM):
	Complex rp_FM_den = nM_cosF + nF_cosM;
	Complex rp_FM = rp_FM_den != 0.f ? (nM_cosF - nF_cosM) / rp_FM_den : 0.f;

	// compute propagation phase and attenuation in the thin film
	Complex exponent = Complex(0.f, 4.f * M_PIf * thickness / waveLength) * nF_cosF;

	// optimization: if the real part of the exponent is very negative, the wave is completely
	// absorbed/attenuated before completing a round trip; we can short-circuit the Airy summation
	// and return the first-surface Fresnel reflection (as no interference from the substrate occurs)
	if (exponent.real() < -15.f) {
		float R_S = std::norm(rs_OF);
		float R_P = std::norm(rp_OF);
		return std::clamp(0.5f * (R_S + R_P), 0.f, 1.f);
	}

	Complex phase = std::exp(exponent);

	// calculate total complex reflection coefficients considering multi-beam interference (Airy equations):
	// s-polarization:
	Complex rs_FM_phase = rs_FM * phase;
	Complex r_S_den = 1.f + rs_OF * rs_FM_phase;
	Complex r_S = r_S_den != 0.f ? (rs_OF + rs_FM_phase) / r_S_den : 0.f;

	// p-polarization:
	Complex rp_FM_phase = rp_FM * phase;
	Complex r_P_den = 1.f + rp_OF * rp_FM_phase;
	Complex r_P = r_P_den != 0.f ? (rp_OF + rp_FM_phase) / r_P_den : 0.f;

	// convert complex amplitude coefficients to energy reflectance: R = |r|^2
	float R_S = std::norm(r_S);
	float R_P = std::norm(r_P);

	// average reflectance for unpolarized light
	float R = 0.5f * (R_S + R_P);
	return std::min(R, 1.f);
}

// ------------------------------------------------------------------------ //

/*static const double XYZ_to_RGB[3][3] = {
 // http://en.wikipedia.org/wiki/CIE_1931_color_space
 // CIE 1931
 { 0.41847,   -0.15866,  -0.082835},
 {-0.091169,   0.25243,   0.015708},
 { 0.0009209, -0.0025498, 0.1786},

 // other common RGB Working Space Matrices: http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
 // Adobe RGB (1998)
 //	{ 2.0413690, -0.5649464, -0.3446944},
 //	{-0.9692660,  1.8760108,  0.0415560},
 //	{ 0.0134474, -0.1183897,  1.0154096},

 //	{ 1.9624274, -0.6105343, -0.3413404},
 //	{-0.9787684,  1.9161415,  0.0334540},
 //	{ 0.0286869, -0.1406752,  1.3487655},

 // CIE RGB
 //	{ 2.3706743, -0.9000405, -0.4706338},
 //	{-0.5138850,  1.4253036,  0.0885814},
 //	{ 0.0052982, -0.0146949,  1.0093968},

 // sRGB
 //	{ 3.2404542, -1.5371385, -0.4985314},
 //	{-0.9692660,  1.8760108,  0.0415560},
 //	{ 0.0556434, -0.2040259,  1.0572252},
 };*/

vec3 xyzToRGB(const vec3 & xyz)
{
	vec3 rgb;

	// CIE RGB
	rgb.x =  2.3706743f * xyz.x - 0.9000405f * xyz.y - 0.4706338f * xyz.z;
	rgb.y = -0.5138850f * xyz.x + 1.4253036f * xyz.y + 0.0885814f * xyz.z;
	rgb.z =  0.0052982f * xyz.x - 0.0146949f * xyz.y + 1.0093968 * xyz.z;

//	rgb.x =  2.3638081f * xyz.x - 0.8676030f * xyz.y - 0.4988161f * xyz.z;
//	rgb.y = -0.5005940f * xyz.x + 1.3962369f * xyz.y + 0.1047562f * xyz.z;
//	rgb.z =  0.0141712f * xyz.x - 0.0306400f * xyz.y + 1.2323842f * xyz.z;

	// sRGB
//	rgb.x =  3.240479f * xyz.x - 1.537150f * xyz.y - 0.498535f * xyz.z;
//	rgb.y = -0.969256f * xyz.x + 1.875991f * xyz.y + 0.041556f * xyz.z;
//	rgb.z =  0.055648f * xyz.x - 0.204043f * xyz.y + 1.057311f * xyz.z;

	rgb.x = std::max(rgb.x, 0.f);
	rgb.y = std::max(rgb.y, 0.f);
	rgb.z = std::max(rgb.z, 0.f);

	return rgb;//ColorUtils::sRGBToLinear(rgb);
}

// ------------------------------------------------------------------------ //

vec2 rejectionSampleDisk(Random & sampler)
{
	vec2 v;

	do {
		v.x = sampler.signedRand();
		v.y = sampler.signedRand();
	} while (glm::length2(v) > 1.f);

	return v;
}

vec3 rejectionSampleBall(Random & sampler)
{
	vec3 v;

	do {
		v.x = sampler.signedRand();
		v.y = sampler.signedRand();
		v.z = sampler.signedRand();
	} while (glm::length2(v) > 1.f);

	return v;
}

vec2 uniformSampleTriangle(Sampler & sampler)
{
	return uniformSampleTriangle(sampler.generate1D(), sampler.generate1D());
}

vec2 uniformSampleDisk(Sampler & sampler)
{
	return uniformSampleDisk(sampler.generate1D(), sampler.generate1D());
}

vec2 concentricSampleDisk(Sampler & sampler)
{
	return concentricSampleDisk(sampler.generate1D(), sampler.generate1D());
}

vec3 uniformSampleHemisphere(Sampler & sampler)
{
	return uniformSampleHemisphere(sampler.generate1D(), sampler.generate1D());
}

vec3 uniformSampleSphere(Sampler & sampler)
{
	return uniformSampleSphere(sampler.generate1D(), sampler.generate1D());
}

// ------------------------------------------------------------------------ //

vec3 getBumpNormal(const vec3 & dpdx, const vec3 & dpdy, const vec3 & dpdz, const TextureImpl * normalMap, const vec2 & tc)
{
	auto nm = normalMap->getNormal(tc);
	return glm::normalize(dpdx * nm.x + dpdy * nm.y + dpdz * nm.z);
}

//void traceBumpMap(Ray & ray, TextureScalarParameterImpl & bump, int numLinearSearchSteps, int numBinarySearchSteps)
//{
//	auto bumpDepth = bump.ScalarParameterImpl::GetScalar();
//	ray.setBump(bump.texture(), bumpDepth);
//	auto & dir = ray.direction();
//	vec3 dirTS(glm::dot(dir, ray.dpdx), glm::dot(dir, ray.dpdy), glm::dot(dir, ray.dpdz));
//	auto dpDN = dirTS.z;
//	dirTS.Normalize();
//	dirTS /= std::fabs(dirTS.z);
//	auto step = dirTS * (1.f / numLinearSearchSteps);
//
//	auto backface = dpDN > 0.f;
//	auto rayZ = backface ? 0.f : 1.f;
//	if (backface)
//	{
//		ray.u -= dirTS.x;
//		ray.v -= dirTS.y;
//	}
//
//	for (int i = 0; i < numLinearSearchSteps; i++)
//	{
//		ray.bumpZ = ray.normalMap->GetHeight(ray);
//		if ((rayZ < ray.bumpZ) ^ backface)
//			break;
//
//		ray.u += step.x;
//		ray.v += step.y;
//		rayZ += step.z;
//	}
//
//	for (int i = 0; i < numBinarySearchSteps; i++)
//	{
//		step *= 0.5f;
//		ray.u -= step.x;
//		ray.v -= step.y;
//		rayZ -= step.z;
//
//		ray.bumpZ = ray.normalMap->GetHeight(ray);
//		if ((rayZ > ray.bumpZ) ^ backface)
//		{
//			ray.u += step.x;
//			ray.v += step.y;
//			rayZ += step.z;
////			pos += step;
//		}
//	}
//
////	ray.tc = pos;
//	ray.tc = ray.textureCoords(0);
//	auto delta = dir * (bumpDepth * (ray.bumpZ - 1.f) / dpDN);
//	ray.pos += delta;
//	ray.normal = getBumpNormal(ray.dpdx, ray.dpdy, ray.dpdz, ray.normalMap, ray.tc);
//}

void traceBumpMap(Ray & ray, TextureScalarParameterImpl & bump, int numLinearSearchSteps, int numBinarySearchSteps)
{
	if (bump.texture()->normalMap().get()) {
		auto normalMap = bump.texture();
		vec3 dpdx, dpdy;
		normalMap->getTangentBinormal(dpdx, dpdy, ray, bump.value());
		ray.normal = getBumpNormal(dpdx, dpdy, ray.normal, normalMap, normalMap->getTC(ray));
		return;
	}

	ray.bumpDepth = bump.value();
	ray.setBump(bump.texture(), ray.bumpDepth);
	const auto & dir = ray.direction();
	vec3 dirTS(glm::dot(dir, ray.dpdx), glm::dot(dir, ray.dpdy), glm::dot(dir, ray.dpdz));
	if (dirTS.z == 0.f) {
		ray.bumpZ = 1.f;
		return;
	}

	auto backface = dirTS.z > 0.f;
//	assert(isfinite(dirTS.x) && isfinite(dirTS.y) && isfinite(dirTS.z));
	auto dpDN = dirTS.z;
	dirTS = glm::normalize(dirTS);
	dirTS /= std::fabs(dirTS.z);
	auto step = dirTS * (1.f / numLinearSearchSteps);
	auto pos = backface ? vec3(ray.bumpTC.x - dirTS.x, ray.bumpTC.y - dirTS.y, 0.f) : vec3(ray.bumpTC.x, ray.bumpTC.y, 1.f);
//	pos += step * generate1D();

	while (numLinearSearchSteps-- > 0) {
		ray.bumpZ = ray.normalMap->getHeight(reinterpret_cast<const vec2 &>(pos));
		if ((pos.z < ray.bumpZ) ^ backface)
			break;

		pos += step;
	}

	while (numBinarySearchSteps-- > 0) {
		step *= 0.5f;
		pos -= step;

		ray.bumpZ = ray.normalMap->getHeight(reinterpret_cast<const vec2 &>(pos));
		if ((pos.z > ray.bumpZ) ^ backface)
			pos += step;
	}

//	{
//		auto dx = ray.dpdu * (bumpDepth / glm::length2(ray.dpdu));
//		dx -= ray.normal * glm::dot(dx, ray.normal);
//		auto dy = ray.dpdv * (bumpDepth / glm::length2(ray.dpdv));
//		dy -= ray.normal * glm::dot(dy, ray.normal);
//		vec3 dirTS2(glm::dot(dir, dx), glm::dot(dir, dy), dpDN);
//		dirTS2.Normalize();
//		dirTS2 /= std::fabs(dirTS2.z);
//		dirTS2 *= (1.f - ray.bumpZ);
//		ray.u += dirTS2.x;
//		ray.v += dirTS2.y;
//	}

	ray.bumpTC = reinterpret_cast<const vec2 &>(pos);

	auto delta = dir * (ray.bumpDepth * (ray.bumpZ - 1.f) / dpDN);
	ray.hitPoint += delta;

	auto dUV = getBarycentricCoords(delta, ray.dpdu(), ray.dpdv());
	ray.hit.u += dUV.x;
	ray.hit.v += dUV.y;

	ray.normal = getBumpNormal(ray.dpdx, ray.dpdy, ray.dpdz, ray.normalMap, ray.bumpTC);
}

//static const float MIN_NORMALS_DP = std::cos(glm::radians(85.f));

bool traceBumpShadow(const vec3 & dir, const Ray & ray, int numLinearSearchSteps, float offset)
{
	if (ray.hitBack) {
		if (ray.bumpZ <= 0.f)
			return true;
	} else {
		if (ray.bumpZ >= 1.f)
			return true;
	}

	vec3 dirTS(glm::dot(dir, ray.dpdx), glm::dot(dir, ray.dpdy), glm::dot(dir, ray.dpdz));
	if (ray.hitBack) {
		if (dirTS.z > 0.f)
			return false;

		if (dirTS.z > -EPS_COSINE)
			return true;
	} else {
		if (dirTS.z < 0.f)
			return false;

		if (dirTS.z < EPS_COSINE)
			return true;
	}

	dirTS = glm::normalize(dirTS);
	dirTS /= std::fabs(dirTS.z);
	auto dz = 1.f - ray.bumpZ;
	auto numSteps = std::max((int)(numLinearSearchSteps * dz), 2);
	auto step = dirTS * (dz / numSteps);
	auto pos = vec3(ray.bumpTC.x, ray.bumpTC.y, ray.bumpZ);
	pos += step * (offset + 0.5f);

	while (numSteps-- > 0) {
		float bumpZ = ray.normalMap->getHeight(reinterpret_cast<const vec2 &>(pos));
		if (ray.hitBack) {
			if (pos.z > bumpZ)
				return false;
		} else {
			if (pos.z < bumpZ)
				return false;
		}

		pos += step;
	}

	return true;
}

bool traceBumpInside(const vec3 & dir, Ray & ray, int numLinearSearchSteps, int numBinarySearchSteps, float offset)
{
	if (ray.hitBack) {
		if (ray.bumpZ <= 0.f)
			return true;
	} else {
		if (ray.bumpZ >= 1.f)
			return true;
	}

	vec3 dirTS(glm::dot(dir, ray.dpdx), glm::dot(dir, ray.dpdy), glm::dot(dir, ray.dpdz));
	if (dirTS.z == 0.f)
		return true;

	auto backface = dirTS.z > 0.f;
	auto dpDN = dirTS.z;
	dirTS = glm::normalize(dirTS);
	dirTS /= std::fabs(dirTS.z);
	auto dz = backface ? (1.f - ray.bumpZ) : ray.bumpZ;
	auto numSteps = std::max((int)(numLinearSearchSteps * dz), 2);
	auto step = dirTS * (dz / numSteps);
	auto pos = vec3(ray.bumpTC.x, ray.bumpTC.y, ray.bumpZ);
	pos += step * (1.f + offset);

	while (numSteps-- > 0) {
		auto bumpZ = ray.normalMap->getHeight(reinterpret_cast<const vec2 &>(pos));
		if (ray.hitBack) {
			if (pos.z < bumpZ) {
				pos += step;
				continue;
			}
		} else {
			if (pos.z > bumpZ) {
				pos += step;
				continue;
			}
		}

		while (numBinarySearchSteps-- > 0) {
			step *= 0.5f;
			pos -= step;

			bumpZ = ray.normalMap->getHeight(reinterpret_cast<const vec2 &>(pos));
			if (ray.hitBack) {
				if (pos.z < bumpZ)
					pos += step;
			} else {
				if (pos.z > bumpZ)
					pos += step;
			}
		}

		auto tfar = ray.bumpDepth * std::max((bumpZ - ray.bumpZ) / dpDN, 0.f);
		if (tfar <= ray.ray.tnear)
			return true;

		ray.ray.tfar = tfar;
		ray.bumpZ = bumpZ;
		ray.bumpTC = reinterpret_cast<const vec2 &>(pos);

		auto delta = dir * ray.ray.tfar;
		ray.hitPoint += delta;

		auto dUV = getBarycentricCoords(delta, ray.dpdu(), ray.dpdv());
		ray.hit.u += dUV.x;
		ray.hit.v += dUV.y;

		ray.normal = getBumpNormal(ray.dpdx, ray.dpdy, ray.dpdz, ray.normalMap, ray.bumpTC);

		return false;
	}

	//assert(backface);

	return true;
}

