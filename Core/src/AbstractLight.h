#pragma once

struct SceneSphere {
	vec3	center; // Center of the scene's bounding sphere
	float	radius; // Radius of the scene's bounding sphere
	float	invRadiusSqr; // 1.f / (radius^2)
};

class Sampler;
struct Ray;

class AbstractLight
{
public:

	/* \brief Illuminates a given point in the scene.
	 *
	 * Given a point and two random samples (e.g., for position on area lights),
	 * this method returns direction from point to light, distance,
	 * pdf of having chosen this direction (e.g., 1 / area).
	 * Optionally also returns pdf of emitting particle in this direction,
	 * and cosine from lights normal (helps with PDF of hitting the light,
	 * but set to 1 for point lights).
	 *
	 * Returns radiance.
	 */
	virtual vec3 illuminate(const vec3 & aReceivingPosition,
							Sampler & sampler,
							vec3 & oDirectionToLight, float & oDistance,
							float & oDirectPdfW, float * oEmissionPdfW = nullptr,
							float * oCosAtLight = nullptr) const = 0;

	// Same as above, but for our fake floor. Calls illuminate() if unsure, overriden for the environment light
	// to take only the top part of the sky into account.
	virtual vec3 illuminateFloor(const vec3 & aReceivingPosition,
							Sampler & sampler,
							vec3 & oDirectionToLight, float & oDistance,
							float & oDirectPdfW, float * oEmissionPdfW = nullptr,
							float * oCosAtLight = nullptr) const = 0;

	/* \brief Emits particle from the light.
	 *
	 * Given two sets of random numbers (e.g., position and direction on area light),
	 * this method generates a position and direction for light particle, along
	 * with the pdf.
	 *
	 * Can also supply pdf (w.r.t. area) of choosing this position when calling
	 * Illuminate. Also provides cosine on the light (this is 1 for point lights etc.).
	 *
	 * Returns "energy" that particle carries
	 */
	virtual vec3 emitParticle(Sampler & sampler,
							  vec3 & oPosition, vec3 & oDirection,
							  float & oEmissionPdfW, float * oDirectPdfA,
							  float * oCosThetaLight) const = 0;

	/* \brief Returns radiance for ray randomly hitting the light
	 *
	 * Given ray direction and hitpoint, it returns radiance.
	 * Can also provide area pdf of sampling hitpoint in Illuminate,
	 * and of emitting particle along the ray (in opposite direction).
	 */
	virtual vec3 getRadiance(const Ray & ray,
							 float * oDirectPdfA = nullptr, float * oEmissionPdfW = nullptr,
							 float * oCosThetaLight = nullptr) const = 0;

	// Whether the light has a finite extent (area, point) or not (directional, env. map)
	virtual bool isFinite() const = 0;

	// Whether the light has delta function (point, directional) or not (area)
	virtual bool isDelta() const = 0;

	virtual unsigned geomID() const = 0;

	virtual void prepare() = 0;
};
