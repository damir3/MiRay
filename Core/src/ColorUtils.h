#pragma once

namespace ColorUtils {

inline float sRGBToLinear(float c)
{
	if (c <= 0.0f) {
		return 0.0f;
	}
	if (c >= 1.0f) {
		return c;
	}
	return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

inline float linearToSRGB(float c)
{
	if (c <= 0.0f) {
		return 0.0f;
	}
	if (c >= 1.0f) {
		return c;
	}
	return (c <= 0.0031308f) ? (c * 12.92f) : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
}

inline vec3 sRGBToLinear(const vec3 & c)
{
	return vec3(sRGBToLinear(c.r), sRGBToLinear(c.g), sRGBToLinear(c.b));
}

inline vec3 linearToSRGB(const vec3 & c)
{
	return vec3(linearToSRGB(c.r), linearToSRGB(c.g), linearToSRGB(c.b));
}

inline vec4 sRGBToLinear(const vec4 & c)
{
	return vec4(sRGBToLinear(c.r), sRGBToLinear(c.g), sRGBToLinear(c.b), c.a);
}

inline vec4 linearToSRGB(const vec4 & c)
{
	return vec4(linearToSRGB(c.r), linearToSRGB(c.g), linearToSRGB(c.b), c.a);
}

inline vec4 premultipliedToStraight(const vec4 & c)
{
	if (c.a <= 0.0f) {
		return vec4(0.0f, 0.0f, 0.0f, c.a);
	}
	const float invAlpha = 1.0f / c.a;
	return vec4(c.r * invAlpha, c.g * invAlpha, c.b * invAlpha, c.a);
}

inline vec4 straightToPremultiplied(const vec4 & c)
{
	return vec4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
}

inline vec3 clamp(const vec3 & c, float minVal = 0.0f, float maxVal = 1.0f)
{
	return glm::clamp(c, minVal, maxVal);
}

inline vec4 clamp(const vec4 & c, float minVal = 0.0f, float maxVal = 1.0f)
{
	return glm::clamp(c, minVal, maxVal);
}

inline vec4 clampRGB(const vec4 & c, float minVal = 0.0f, float maxVal = 1.0f)
{
	return vec4(glm::clamp(vec3(c), minVal, maxVal), c.a);
}

inline QColor toQColor(const glm::vec4 &clr)
{
	return QColor::fromRgbF(clr.r, clr.g, clr.b);
}

inline glm::vec4 fromQColor(const QColor &clr)
{
	return glm::vec4(clr.redF(), clr.greenF(), clr.blueF(), 1.f);
}

CORE_EXPORT QString toString(const glm::vec4 & c);
CORE_EXPORT glm::vec4 fromString(QString str, const glm::vec4 & def = glm::vec4(0.0f));

} // namespace ColorUtils
