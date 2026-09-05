#include "../Core/Application.h"
#include "../Core/src/ImageImpl.h"

TEST(TestImages, LoadNoiseJpg)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);
	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	const QString path = ":/noise.jpg";

	auto img = imgManager->loadImage(path, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, img);
	EXPECT_EQ(256, img->width());
	EXPECT_EQ(256, img->height());
	EXPECT_EQ(vec2(600.f), img->dpi());
	EXPECT_EQ(eImageFormat::Grayscale, img->format());
	EXPECT_EQ(eImageDataType::Byte, img->dataType());
	EXPECT_EQ(eImageColorSpace::sRGB, img->colorSpace());

	auto imgSrgb = imgManager->loadImage(path, false, eImageColorSpace::sRGB);
	ASSERT_NE(nullptr, imgSrgb);
	EXPECT_EQ(256, imgSrgb->width());
	EXPECT_EQ(256, imgSrgb->height());
	EXPECT_EQ(vec2(600.f), imgSrgb->dpi());
	EXPECT_EQ(eImageFormat::Grayscale, imgSrgb->format());
	EXPECT_EQ(eImageDataType::Byte, imgSrgb->dataType());
	EXPECT_EQ(eImageColorSpace::sRGB, imgSrgb->colorSpace());

	auto imgLinear = imgManager->loadImage(path, false, eImageColorSpace::Linear);
	ASSERT_NE(nullptr, imgLinear);
	EXPECT_EQ(256, imgLinear->width());
	EXPECT_EQ(256, imgLinear->height());
	EXPECT_EQ(vec2(600.f), imgLinear->dpi());
	EXPECT_EQ(eImageFormat::Grayscale, imgLinear->format());
	EXPECT_EQ(eImageDataType::Float, imgLinear->dataType());
	EXPECT_EQ(eImageColorSpace::Linear, imgLinear->colorSpace());

	EXPECT_GT(img->getPixel(50, 50).r, 0.05f);
	EXPECT_GT(imgSrgb->getPixel(50, 50).r, 0.05f);
	EXPECT_GT(imgLinear->getPixel(50, 50).r, 0.05f);
}

TEST(TestImages, Load4x4Hdr)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);
	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	const QString path = ":/4x4.hdr";

	auto img = imgManager->loadImage(path, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, img);
	EXPECT_EQ(4, img->width());
	EXPECT_EQ(4, img->height());
	EXPECT_EQ(vec2(72.f), img->dpi());
	EXPECT_EQ(eImageFormat::RGB, img->format());
	EXPECT_EQ(eImageDataType::Float, img->dataType());
	EXPECT_EQ(eImageColorSpace::Linear, img->colorSpace());

	auto imgLinear = imgManager->loadImage(path, false, eImageColorSpace::Linear);
	ASSERT_NE(nullptr, imgLinear);
	EXPECT_EQ(4, imgLinear->width());
	EXPECT_EQ(4, imgLinear->height());
	EXPECT_EQ(vec2(72.f), imgLinear->dpi());
	EXPECT_EQ(eImageFormat::RGB, imgLinear->format());
	EXPECT_EQ(eImageDataType::Float, imgLinear->dataType());
	EXPECT_EQ(eImageColorSpace::Linear, imgLinear->colorSpace());

	auto imgSrgb = imgManager->loadImage(path, false, eImageColorSpace::sRGB);
	ASSERT_NE(nullptr, imgSrgb);

	EXPECT_EQ(4, imgSrgb->width());
	EXPECT_EQ(4, imgSrgb->height());
	EXPECT_EQ(vec2(72.f), imgSrgb->dpi());
	EXPECT_EQ(eImageFormat::RGB, imgSrgb->format());
	EXPECT_EQ(eImageDataType::Byte, imgSrgb->dataType());
	EXPECT_EQ(eImageColorSpace::sRGB, imgSrgb->colorSpace());

	// Verify colors row by row:
	// y = 0: Red
	// y = 1: Green
	// y = 2: Blue
	// y = 3: White
	auto checkHdrColors = [](const ImagePtr & image) {
		ASSERT_NE(nullptr, image);
		for (int x = 0; x < 4; ++x) {
			const auto p0 = image->getPixel(x, 0);
			EXPECT_GT(p0.r, 0.5f);
			EXPECT_LT(p0.g, 0.01f);
			EXPECT_LT(p0.b, 0.01f);

			const auto p1 = image->getPixel(x, 1);
			EXPECT_LT(p1.r, 0.01f);
			EXPECT_GT(p1.g, 0.5f);
			EXPECT_LT(p1.b, 0.01f);

			const auto p2 = image->getPixel(x, 2);
			EXPECT_LT(p2.r, 0.01f);
			EXPECT_LT(p2.g, 0.01f);
			EXPECT_GT(p2.b, 0.5f);

			const auto p3 = image->getPixel(x, 3);
			EXPECT_GT(p3.r, 0.5f);
			EXPECT_GT(p3.g, 0.5f);
			EXPECT_GT(p3.b, 0.5f);
		}
	};

	// Check eImageColorSpace::Unknown
	checkHdrColors(img);

	// Check eImageColorSpace::Linear
	checkHdrColors(imgLinear);

	// Both Unknown and Linear should match exactly for HDR
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			EXPECT_FLOAT_EQ(img->getPixel(x, y).r, imgLinear->getPixel(x, y).r);
			EXPECT_FLOAT_EQ(img->getPixel(x, y).g, imgLinear->getPixel(x, y).g);
			EXPECT_FLOAT_EQ(img->getPixel(x, y).b, imgLinear->getPixel(x, y).b);
		}
	}

	// Verify sRGB version has expected colors as well
	for (int x = 0; x < 4; ++x) {
		const auto p0 = imgSrgb->getPixel(x, 0);
		EXPECT_GT(p0.r, 0.5f);
		EXPECT_LT(p0.g, 0.05f);
		EXPECT_LT(p0.b, 0.05f);

		const auto p1 = imgSrgb->getPixel(x, 1);
		EXPECT_LT(p1.r, 0.05f);
		EXPECT_GT(p1.g, 0.5f);
		EXPECT_LT(p1.b, 0.05f);

		const auto p2 = imgSrgb->getPixel(x, 2);
		EXPECT_LT(p2.r, 0.05f);
		EXPECT_LT(p2.g, 0.05f);
		EXPECT_GT(p2.b, 0.5f);

		const auto p3 = imgSrgb->getPixel(x, 3);
		EXPECT_GT(p3.r, 0.5f);
		EXPECT_GT(p3.g, 0.5f);
		EXPECT_GT(p3.b, 0.5f);
	}
}

TEST(TestImages, LoadGreyscaleExr)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);
	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	const QString path = ":/greyscale.exr";

	auto img = imgManager->loadImage(path, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, img);
	EXPECT_EQ(32, img->width());
	EXPECT_EQ(32, img->height());
	EXPECT_EQ(eImageFormat::Grayscale, img->format());
	EXPECT_EQ(eImageDataType::Float, img->dataType());
	EXPECT_EQ(eImageColorSpace::Linear, img->colorSpace());

	auto imgLinear = imgManager->loadImage(path, false, eImageColorSpace::Linear);
	ASSERT_NE(nullptr, imgLinear);
	EXPECT_EQ(32, imgLinear->width());
	EXPECT_EQ(32, imgLinear->height());
	EXPECT_EQ(eImageFormat::Grayscale, imgLinear->format());
	EXPECT_EQ(eImageDataType::Float, imgLinear->dataType());
	EXPECT_EQ(eImageColorSpace::Linear, imgLinear->colorSpace());

	auto imgSrgb = imgManager->loadImage(path, false, eImageColorSpace::sRGB);
	ASSERT_NE(nullptr, imgSrgb);
	EXPECT_EQ(32, imgSrgb->width());
	EXPECT_EQ(32, imgSrgb->height());
	EXPECT_EQ(eImageFormat::Grayscale, imgSrgb->format());
	EXPECT_EQ(eImageDataType::Byte, imgSrgb->dataType());
	EXPECT_EQ(eImageColorSpace::sRGB, imgSrgb->colorSpace());

	// Verify all pixels are monochromatic (r == g == b, alpha == 1.0f)
	for (int y = 0; y < 32; ++y) {
		for (int x = 0; x < 32; ++x) {
			const auto p = img->getPixel(x, y);
			EXPECT_FLOAT_EQ(p.r, p.g);
			EXPECT_FLOAT_EQ(p.g, p.b);
			EXPECT_FLOAT_EQ(1.0f, p.a);
			EXPECT_GT(p.r, 0.0f);
			EXPECT_LE(p.r, 1.0f);
		}
	}
}

