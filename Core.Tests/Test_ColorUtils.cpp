#include "../Core/src/ColorUtils.h"

TEST(TestColorUtils, BoundaryConditions)
{
	EXPECT_FLOAT_EQ(0.0f, ColorUtils::sRGBToLinear(0.0f));
	EXPECT_FLOAT_EQ(1.0f, ColorUtils::sRGBToLinear(1.0f));

	EXPECT_FLOAT_EQ(0.0f, ColorUtils::linearToSRGB(0.0f));
	EXPECT_FLOAT_EQ(1.0f, ColorUtils::linearToSRGB(1.0f));
}

TEST(TestColorUtils, LinearThreshold)
{
	// At the IEC 61966-2-1 transition point
	const float srgbThreshold = 0.04045f;
	const float expectedLinear = srgbThreshold / 12.92f;
	EXPECT_NEAR(expectedLinear, ColorUtils::sRGBToLinear(srgbThreshold), 1e-6f);

	const float linearThreshold = 0.0031308f;
	const float expectedSRGB = linearThreshold * 12.92f;
	EXPECT_NEAR(expectedSRGB, ColorUtils::linearToSRGB(linearThreshold), 1e-5f);
}

TEST(TestColorUtils, KnownValues)
{
	// Middle gray sRGB 0.5 -> linear ~ 0.214041
	EXPECT_NEAR(0.214041f, ColorUtils::sRGBToLinear(0.5f), 1e-4f);

	// Standard 18% gray card (0.18 linear) -> sRGB ~ 0.46135
	EXPECT_NEAR(0.46135f, ColorUtils::linearToSRGB(0.18f), 1e-4f);
}

TEST(TestColorUtils, RoundTrip)
{
	for (int i = 0; i <= 100; ++i) {
		float orig = static_cast<float>(i) / 100.0f;
		float lin = ColorUtils::sRGBToLinear(orig);
		float srgb = ColorUtils::linearToSRGB(lin);
		EXPECT_NEAR(orig, srgb, 1e-5f);
	}
}

TEST(TestColorUtils, HDRAndNegative)
{
	// Negative values clamped to zero
	EXPECT_FLOAT_EQ(0.0f, ColorUtils::sRGBToLinear(-0.5f));
	EXPECT_FLOAT_EQ(0.0f, ColorUtils::linearToSRGB(-0.5f));

	// HDR values (> 1.0) remain linear
	EXPECT_FLOAT_EQ(2.5f, ColorUtils::sRGBToLinear(2.5f));
	EXPECT_FLOAT_EQ(3.0f, ColorUtils::linearToSRGB(3.0f));
}

TEST(TestColorUtils, VectorConversions)
{
	vec3 linRGB(0.18f, 0.5f, 1.0f);
	vec3 srgbRGB = ColorUtils::linearToSRGB(linRGB);
	EXPECT_NEAR(0.46135f, srgbRGB.r, 1e-4f);
	EXPECT_FLOAT_EQ(1.0f, srgbRGB.b);

	vec3 recoveredLin = ColorUtils::sRGBToLinear(srgbRGB);
	EXPECT_NEAR(linRGB.r, recoveredLin.r, 1e-5f);
	EXPECT_NEAR(linRGB.g, recoveredLin.g, 1e-5f);
	EXPECT_NEAR(linRGB.b, recoveredLin.b, 1e-5f);

	// Vec4 with alpha
	vec4 linRGBA(0.18f, 0.5f, 1.0f, 0.75f);
	vec4 srgbRGBA = ColorUtils::linearToSRGB(linRGBA);
	EXPECT_FLOAT_EQ(0.75f, srgbRGBA.a);

	vec4 recoveredRGBA = ColorUtils::sRGBToLinear(srgbRGBA);
	EXPECT_FLOAT_EQ(0.75f, recoveredRGBA.a);
	EXPECT_NEAR(linRGBA.r, recoveredRGBA.r, 1e-5f);
}

TEST(TestColorUtils, ScalarConversions)
{
	vec3 srgb(0.5f, 0.5f, 0.5f);
	vec3 lin = ColorUtils::sRGBToLinear(srgb);
	EXPECT_NEAR(0.214041f, lin.r, 1e-4f);

	// Scalar intensity
	EXPECT_NEAR(0.214041f, ColorUtils::sRGBToLinear(0.5f), 1e-4f);
	EXPECT_FLOAT_EQ(2.0f, ColorUtils::sRGBToLinear(2.0f));
}

TEST(TestColorUtils, AlphaConversions)
{
	// 1. Straight to Premultiplied
	vec4 straight(0.6f, 0.8f, 1.0f, 0.5f);
	vec4 premul = ColorUtils::straightToPremultiplied(straight);
	EXPECT_FLOAT_EQ(0.3f, premul.r);
	EXPECT_FLOAT_EQ(0.4f, premul.g);
	EXPECT_FLOAT_EQ(0.5f, premul.b);
	EXPECT_FLOAT_EQ(0.5f, premul.a);

	// 2. Premultiplied to Straight
	vec4 recoveredStraight = ColorUtils::premultipliedToStraight(premul);
	EXPECT_FLOAT_EQ(straight.r, recoveredStraight.r);
	EXPECT_FLOAT_EQ(straight.g, recoveredStraight.g);
	EXPECT_FLOAT_EQ(straight.b, recoveredStraight.b);
	EXPECT_FLOAT_EQ(straight.a, recoveredStraight.a);

	// 3. Fully opaque (alpha = 1.0f)
	vec4 opaque(0.2f, 0.4f, 0.6f, 1.0f);
	vec4 opaquePremul = ColorUtils::straightToPremultiplied(opaque);
	EXPECT_FLOAT_EQ(opaque.r, opaquePremul.r);
	EXPECT_FLOAT_EQ(opaque.g, opaquePremul.g);
	EXPECT_FLOAT_EQ(opaque.b, opaquePremul.b);
	EXPECT_FLOAT_EQ(opaque.a, opaquePremul.a);

	vec4 opaqueStraight = ColorUtils::premultipliedToStraight(opaquePremul);
	EXPECT_FLOAT_EQ(opaque.r, opaqueStraight.r);
	EXPECT_FLOAT_EQ(opaque.g, opaqueStraight.g);
	EXPECT_FLOAT_EQ(opaque.b, opaqueStraight.b);
	EXPECT_FLOAT_EQ(opaque.a, opaqueStraight.a);

	// 4. Fully transparent (alpha = 0.0f)
	vec4 zeroAlphaStraight(0.5f, 0.7f, 0.9f, 0.0f);
	vec4 zeroAlphaPremul = ColorUtils::straightToPremultiplied(zeroAlphaStraight);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaPremul.r);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaPremul.g);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaPremul.b);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaPremul.a);

	vec4 zeroAlphaRecovered = ColorUtils::premultipliedToStraight(zeroAlphaPremul);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaRecovered.r);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaRecovered.g);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaRecovered.b);
	EXPECT_FLOAT_EQ(0.0f, zeroAlphaRecovered.a);

	// 5. Negative alpha (should not divide by zero or cause NaN)
	vec4 negativeAlpha(0.5f, 0.5f, 0.5f, -0.5f);
	vec4 negStraight = ColorUtils::premultipliedToStraight(negativeAlpha);
	EXPECT_FLOAT_EQ(0.0f, negStraight.r);
	EXPECT_FLOAT_EQ(0.0f, negStraight.g);
	EXPECT_FLOAT_EQ(0.0f, negStraight.b);
	EXPECT_FLOAT_EQ(-0.5f, negStraight.a);

	// 6. Round trip for various alpha values
	for (int i = 1; i <= 100; ++i) {
		float a = static_cast<float>(i) / 100.0f;
		vec4 orig(0.2f, 0.5f, 0.8f, a);
		vec4 p = ColorUtils::straightToPremultiplied(orig);
		vec4 s = ColorUtils::premultipliedToStraight(p);
		EXPECT_NEAR(orig.r, s.r, 1e-5f);
		EXPECT_NEAR(orig.g, s.g, 1e-5f);
		EXPECT_NEAR(orig.b, s.b, 1e-5f);
		EXPECT_FLOAT_EQ(orig.a, s.a);
	}
}

TEST(TestColorUtils, Clamp)
{
	// Default range [0.0f, 1.0f]
	vec4 v4(-0.5f, 0.4f, 1.5f, 2.0f);
	vec4 clamped4 = ColorUtils::clamp(v4);
	EXPECT_FLOAT_EQ(0.0f, clamped4.r);
	EXPECT_FLOAT_EQ(0.4f, clamped4.g);
	EXPECT_FLOAT_EQ(1.0f, clamped4.b);
	EXPECT_FLOAT_EQ(1.0f, clamped4.a);

	// Custom range
	vec4 clampedCustom = ColorUtils::clamp(v4, 0.1f, 0.8f);
	EXPECT_FLOAT_EQ(0.1f, clampedCustom.r);
	EXPECT_FLOAT_EQ(0.4f, clampedCustom.g);
	EXPECT_FLOAT_EQ(0.8f, clampedCustom.b);
	EXPECT_FLOAT_EQ(0.8f, clampedCustom.a);

	// Vec3 overload
	vec3 v3(-1.0f, 0.7f, 3.0f);
	vec3 clamped3 = ColorUtils::clamp(v3);
	EXPECT_FLOAT_EQ(0.0f, clamped3.r);
	EXPECT_FLOAT_EQ(0.7f, clamped3.g);
	EXPECT_FLOAT_EQ(1.0f, clamped3.b);

	// clampRGB: clamps RGB while preserving alpha
	vec4 v4Alpha(-0.5f, 0.4f, 1.5f, 2.0f);
	vec4 clampedRGB = ColorUtils::clampRGB(v4Alpha);
	EXPECT_FLOAT_EQ(0.0f, clampedRGB.r);
	EXPECT_FLOAT_EQ(0.4f, clampedRGB.g);
	EXPECT_FLOAT_EQ(1.0f, clampedRGB.b);
	EXPECT_FLOAT_EQ(2.0f, clampedRGB.a); // alpha untouched
}

TEST(TestColorUtils, QColorConversions)
{
	vec4 col(0.2f, 0.4f, 0.6f, 1.0f);
	QColor qcol = ColorUtils::toQColor(col);
	EXPECT_NEAR(0.2f, qcol.redF(), 0.01f);
	EXPECT_NEAR(0.4f, qcol.greenF(), 0.01f);
	EXPECT_NEAR(0.6f, qcol.blueF(), 0.01f);

	vec4 back = ColorUtils::fromQColor(qcol);
	EXPECT_NEAR(0.2f, back.r, 0.01f);
	EXPECT_NEAR(0.4f, back.g, 0.01f);
	EXPECT_NEAR(0.6f, back.b, 0.01f);
	EXPECT_FLOAT_EQ(1.0f, back.a);
}

TEST(TestColorUtils, StringConversions)
{
	vec4 col3(1.0f, 0.5f, 0.25f, 1.0f);
	EXPECT_EQ("1 0.5 0.25", ColorUtils::toString(col3));

	vec4 col4(1.0f, 0.5f, 0.25f, 0.8f);
	EXPECT_EQ("1 0.5 0.25 0.8", ColorUtils::toString(col4));

	// Hex colors (3-digit and 6-digit)
	EXPECT_EQ(glm::vec4(1, 0, 0, 1), ColorUtils::fromString("#f00"));
	EXPECT_EQ(glm::vec4(0, 0, 1, 1), ColorUtils::fromString("#0000ff"));
	EXPECT_EQ(glm::vec4(0, 0, 1, 1), ColorUtils::fromString("#0000FF"));

	// CSS named colors (standard keyword names)
	EXPECT_EQ(glm::vec4(1, 0, 0, 1), ColorUtils::fromString("red"));
	EXPECT_EQ(glm::vec4(1, 0, 0, 1), ColorUtils::fromString("RED"));
	EXPECT_EQ(glm::vec4(0, 1, 0, 1), ColorUtils::fromString("lime"));
	EXPECT_EQ(glm::vec4(0, 0, 1, 1), ColorUtils::fromString("blue"));
	EXPECT_EQ(glm::vec4(1, 1, 1, 1), ColorUtils::fromString("white"));
	EXPECT_EQ(glm::vec4(0, 0, 0, 1), ColorUtils::fromString("black"));
	EXPECT_EQ(glm::vec4(1, 1, 0, 1), ColorUtils::fromString("yellow"));
	EXPECT_EQ(glm::vec4(0, 1, 1, 1), ColorUtils::fromString("cyan"));
	EXPECT_EQ(glm::vec4(0, 1, 1, 1), ColorUtils::fromString("aqua"));
	EXPECT_EQ(glm::vec4(1, 0, 1, 1), ColorUtils::fromString("magenta"));
	EXPECT_EQ(glm::vec4(1, 0, 1, 1), ColorUtils::fromString("fuchsia"));

	auto orange = ColorUtils::fromString("orange");
	EXPECT_FLOAT_EQ(1.0f, orange.r);
	EXPECT_NEAR(165.0f / 255.0f, orange.g, 1e-4f);
	EXPECT_FLOAT_EQ(0.0f, orange.b);
	EXPECT_FLOAT_EQ(1.0f, orange.a);

	// Space-delimited numbers (RGB and RGBA)
	EXPECT_EQ(glm::vec4(1, 0, 0, 1), ColorUtils::fromString("1 0 0"));
	EXPECT_EQ(glm::vec4(0.2f, 0.4f, 0.6f, 0.8f), ColorUtils::fromString("0.2 0.4 0.6 0.8"));

	// Fallback for invalid values
	EXPECT_EQ(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), ColorUtils::fromString("invalid", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)));
	EXPECT_EQ(glm::vec4(0.0f), ColorUtils::fromString(""));
}
