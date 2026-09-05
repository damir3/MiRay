#include "../Core/src/Utils.h"
#include "../Core/src/ColorUtils.h"
#include "../Shared/Utils/FileUtils.h"
#include "../Core/Application.h"

TEST(TestUtils, ColorFromString1)
{
	EXPECT_EQ(glm::vec4(0, 0, 1, 1), ColorUtils::fromString("#0000ff", glm::vec4(1, 1, 1, 1)));
}

TEST(TestUtils, ColorFromString2)
{
	EXPECT_EQ(glm::vec4(1, 0, 0, 1), ColorUtils::fromString("#ff0000", glm::vec4(1, 1, 1, 1)));
}

TEST(TestUtils, ColorFromString3)
{
	auto clr = ColorUtils::fromString("#ff8000", glm::vec4(1, 1, 1, 1));
	EXPECT_FLOAT_EQ(1, clr.r);
	EXPECT_NEAR(0.5f, clr.g, 0.02f);
	EXPECT_FLOAT_EQ(0, clr.b);
	EXPECT_FLOAT_EQ(1, clr.a);
}

TEST(TestUtils, ColorFromString_Invalid1)
{
	EXPECT_EQ(glm::vec4(0, 1, 1, 1), ColorUtils::fromString("#12345", glm::vec4(0, 1, 1, 1)));
}

TEST(TestUtils, ColorFromString_Invalid2)
{
	EXPECT_EQ(glm::vec4(1, 1, 1, 1), ColorUtils::fromString("#stuff", glm::vec4(1, 1, 1, 1)));
}

TEST(TestUtils, generateUniqueFileNames_AllDifferent)
{
	QSet<QString> input;
	input.insert("miray:///folder 1/image 1.png");
	input.insert("miray:///folder 2/image 2.png");

	QMap<QString, QString> res = generateUniqueFileNamesForResourcesCollection(input, {});

	ASSERT_EQ(2, res.size());
	ASSERT_TRUE(res.contains("miray:///folder 1/image 1.png"));
	ASSERT_TRUE(res.contains("miray:///folder 2/image 2.png"));

	EXPECT_EQ("image 1.png", res["miray:///folder 1/image 1.png"]);
	EXPECT_EQ("image 2.png", res["miray:///folder 2/image 2.png"]);
}

TEST(TestUtils, generateUniqueFileNames_SomeAreTheSame)
{
	QSet<QString> input;
	input.insert("miray:///folder 1/image.png");
	input.insert("miray:///folder 2/image.png");

	QMap<QString, QString> res = generateUniqueFileNamesForResourcesCollection(input, {});

	ASSERT_EQ(2, res.size());
	ASSERT_TRUE(res.contains("miray:///folder 1/image.png"));
	ASSERT_TRUE(res.contains("miray:///folder 2/image.png"));

	EXPECT_EQ("image.png", res["miray:///folder 1/image.png"]);
	EXPECT_EQ("image-1.png", res["miray:///folder 2/image.png"]);
}

TEST(TestUtils, generateUniqueFileNames_SameNamesDifferentCase)
{
	QSet<QString> input;
	input.insert("miray:///folder 1/Image.png");
	input.insert("miray:///folder 2/image.png");

	QMap<QString, QString> res = generateUniqueFileNamesForResourcesCollection(input, {});

	ASSERT_EQ(2, res.size());
	ASSERT_TRUE(res.contains("miray:///folder 1/Image.png"));
	ASSERT_TRUE(res.contains("miray:///folder 2/image.png"));

	EXPECT_EQ("Image.png", res["miray:///folder 1/Image.png"]);
	EXPECT_EQ("image-1.png", res["miray:///folder 2/image.png"]);
}

TEST(TestUtils, generateUniqueFileNames_AvoidNames)
{
	QSet<QString> input;
	input.insert(":/folder 1/image.png");
	input.insert(":/folder 2/image.png");

	QMap<QString, QString> res = generateUniqueFileNamesForResourcesCollection(input, {"Image.PNG"});

	ASSERT_EQ(2, res.size());
	ASSERT_TRUE(res.contains(":/folder 1/image.png"));
	ASSERT_TRUE(res.contains(":/folder 2/image.png"));

	bool bCase1 = "image-1.png" == res[":/folder 1/image.png"] && "image-2.png" == res[":/folder 2/image.png"];
	bool bCase2 = "image-2.png" == res[":/folder 1/image.png"] && "image-1.png" == res[":/folder 2/image.png"];

	EXPECT_TRUE(bCase1 ^ bCase2);
}

TEST(TestUtils, MapOldEnvironmentMapsToNewOnes)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);

	// check mapping of old files
	ASSERT_EQ("Environment 01.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Environment 1.hdr")).fileName());
	ASSERT_EQ("Environment 02.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Environment 2.hdr")).fileName());

	ASSERT_EQ("Studio 1.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Studio 1.hdr")).fileName());
	ASSERT_EQ("Studio 2.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Studio 2.hdr")).fileName());
	ASSERT_EQ("Studio 3.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Studio 3.hdr")).fileName());
	ASSERT_EQ("Studio 4.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Studio 4.hdr")).fileName());
	ASSERT_EQ("Studio 5.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Studio 5.hdr")).fileName());

	// some dummy files that were never mapped
	ASSERT_EQ("Environment 3.hdr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Environment 3.hdr")).fileName());
	ASSERT_EQ("Studio 6.hdr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Studio 6.hdr")).fileName());
	ASSERT_EQ("Room 1.exr", QFileInfo(app.getFullResourcePath("miray:///Library/Textures/Environment/Room 1.exr")).fileName());
}

TEST(TestUtils, SaveAndLoadImage)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);

	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	int w = 32;
	int h = 32;

	// 1. Create a 32x32 RGBA Byte image
	auto img = imgManager->createImage(w, h, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::sRGB);
	ASSERT_NE(nullptr, img);

	// Fill it with a red pattern
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			img->setPixel(x, y, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // solid red
		}
	}

	// 2. File path/name
	QString tempPngPath = QDir::temp().filePath("test_save_image.png");

	// 3. Save as PNG
	QByteArray pngData = imgManager->saveImage(img, "png", 1.0f);
	EXPECT_FALSE(pngData.isEmpty());

	// Write PNG bytes to file to verify loading
	writeFile(tempPngPath, pngData);

	// 4. Load it back (fallback = false)
	auto loadedPng = imgManager->loadImage(tempPngPath, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, loadedPng);
	EXPECT_EQ(w, loadedPng->width());
	EXPECT_EQ(h, loadedPng->height());

	// Check a pixel in the middle
	auto pixel = loadedPng->getPixel(16, 16);
	EXPECT_NEAR(1.0f, pixel.r, 0.01f);
	EXPECT_NEAR(0.0f, pixel.g, 0.01f);
	EXPECT_NEAR(0.0f, pixel.b, 0.01f);
	EXPECT_NEAR(1.0f, pixel.a, 0.01f);

	// Clean up
	QFile::remove(tempPngPath);

	// 5. Test with JPEG
	QString tempJpgPath = QDir::temp().filePath("test_save_image.jpg");

	QByteArray jpgData = imgManager->saveImage(img, "jpg", 0.9f); // 90% quality
	EXPECT_FALSE(jpgData.isEmpty());

	writeFile(tempJpgPath, jpgData);

	// Load it back
	auto loadedJpg = imgManager->loadImage(tempJpgPath, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, loadedJpg);
	EXPECT_EQ(w, loadedJpg->width());
	EXPECT_EQ(h, loadedJpg->height());

	// JPEG is lossy, but red should still be dominated
	auto pixelJpg = loadedJpg->getPixel(16, 16);
	EXPECT_NEAR(1.0f, pixelJpg.r, 0.05f);
	EXPECT_NEAR(0.0f, pixelJpg.g, 0.05f);
	EXPECT_NEAR(0.0f, pixelJpg.b, 0.05f);

	// Clean up
	QFile::remove(tempJpgPath);

	// 6. Test with EXR (Float RGBA)
	auto floatImg = imgManager->createImage(w, h, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, floatImg);

	// Fill with HDR values (values > 1.0f)
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			floatImg->setPixel(x, y, glm::vec4(0.5f, 1.5f, 2.5f, 1.0f));
		}
	}

	QString tempExrPath = QDir::temp().filePath("test_save_image.exr");

	QByteArray exrData = imgManager->saveImage(floatImg, "exr", 1.0f);
	EXPECT_FALSE(exrData.isEmpty());

	writeFile(tempExrPath, exrData);

	// Load back
	auto loadedExr = imgManager->loadImage(tempExrPath, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, loadedExr);
	EXPECT_EQ(w, loadedExr->width());
	EXPECT_EQ(h, loadedExr->height());

	auto pixelExr = loadedExr->getPixel(16, 16);
	EXPECT_NEAR(0.5f, pixelExr.r, 0.01f);
	EXPECT_NEAR(1.5f, pixelExr.g, 0.01f);
	EXPECT_NEAR(2.5f, pixelExr.b, 0.01f);
	EXPECT_NEAR(1.0f, pixelExr.a, 0.01f);

	QFile::remove(tempExrPath);

	// 6b. Test with HDR Grayscale (Float Grayscale)
	auto floatImgGray = imgManager->createImage(w, h, eImageFormat::Grayscale, eImageDataType::Float, eImageColorSpace::Linear);
	ASSERT_NE(nullptr, floatImgGray);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			floatImgGray->setPixel(x, y, glm::vec4(0.75f, 0.75f, 0.75f, 1.0f));
		}
	}

	QString tempExrGrayPath = QDir::temp().filePath("test_save_image_gray.exr");
	QByteArray exrGrayData = imgManager->saveImage(floatImgGray, "exr", 1.0f);
	EXPECT_FALSE(exrGrayData.isEmpty());
	writeFile(tempExrGrayPath, exrGrayData);

	auto loadedExrGray = imgManager->loadImage(tempExrGrayPath, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, loadedExrGray);
	EXPECT_EQ(w, loadedExrGray->width());
	EXPECT_EQ(h, loadedExrGray->height());
	EXPECT_EQ(eImageFormat::Grayscale, loadedExrGray->format());
	EXPECT_EQ(eImageDataType::Float, loadedExrGray->dataType());
	EXPECT_EQ(eImageColorSpace::Linear, loadedExrGray->colorSpace());

	auto pixelExrGray = loadedExrGray->getPixel(16, 16);
	EXPECT_NEAR(0.75f, pixelExrGray.r, 0.01f);
	EXPECT_NEAR(0.75f, pixelExrGray.g, 0.01f);
	EXPECT_NEAR(0.75f, pixelExrGray.b, 0.01f);
	EXPECT_NEAR(1.0f, pixelExrGray.a, 0.01f);

	QFile::remove(tempExrGrayPath);

	// Verify greyscale.exr resource loads as monochromatic Grayscale Float
	auto brushedImg = imgManager->loadImage(":/greyscale.exr", false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, brushedImg);
	EXPECT_EQ(eImageFormat::Grayscale, brushedImg->format());
	EXPECT_EQ(eImageDataType::Float, brushedImg->dataType());
	EXPECT_EQ(eImageColorSpace::Linear, brushedImg->colorSpace());
	for (int y = 0; y < brushedImg->height(); y += 8) {
		for (int x = 0; x < brushedImg->width(); x += 8) {
			auto p = brushedImg->getPixel(x, y);
			EXPECT_FLOAT_EQ(p.r, p.g);
			EXPECT_FLOAT_EQ(p.g, p.b);
			EXPECT_FLOAT_EQ(1.0f, p.a);
		}
	}

	// 7. Test with HDR (Float RGB)
	auto floatImgRGB = imgManager->createImage(w, h, eImageFormat::RGB, eImageDataType::Float, eImageColorSpace::Linear);
	ASSERT_NE(nullptr, floatImgRGB);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			floatImgRGB->setPixel(x, y, glm::vec4(0.5f, 1.5f, 2.5f, 1.0f));
		}
	}

	QString tempHdrPath = QDir::temp().filePath("test_save_image.hdr");

	QByteArray hdrData = imgManager->saveImage(floatImgRGB, "hdr", 1.0f);
	EXPECT_FALSE(hdrData.isEmpty());

	writeFile(tempHdrPath, hdrData);

	// Load back
	auto loadedHdr = imgManager->loadImage(tempHdrPath, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, loadedHdr);
	EXPECT_EQ(w, loadedHdr->width());
	EXPECT_EQ(h, loadedHdr->height());

	// HDR uses RGBE format which has slightly larger error margin, but still preserves high range
	auto pixelHdr = loadedHdr->getPixel(16, 16);
	EXPECT_NEAR(0.5f, pixelHdr.r, 0.05f);
	EXPECT_NEAR(1.5f, pixelHdr.g, 0.05f);
	EXPECT_NEAR(2.5f, pixelHdr.b, 0.05f);

	QFile::remove(tempHdrPath);
}

TEST(TestUtils, ImageColorConversions)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);

	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	int w = 16;
	int h = 16;

	// Create a middle-gray sRGB image
	auto img = imgManager->createImage(w, h, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::sRGB);
	ASSERT_NE(nullptr, img);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			img->setPixel(x, y, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
		}
	}

	QString tempPngPath = QDir::temp().filePath("test_conversion_image.png");
	QByteArray pngData = imgManager->saveImage(img, "png", 1.0f);
	EXPECT_FALSE(pngData.isEmpty());
	writeFile(tempPngPath, pngData);

	// 1. Load with None (standard sRGB preserved)
	auto loadedNone = imgManager->loadImage(tempPngPath, false, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, loadedNone);
	auto pixelNone = loadedNone->getPixel(8, 8);
	EXPECT_NEAR(0.5f, pixelNone.r, 0.01f);

	// 2. Load with Linear (skcms converts sRGB -> linear, ~0.214)
	auto loadedLinear = imgManager->loadImage(tempPngPath, false, eImageColorSpace::Linear);
	ASSERT_NE(nullptr, loadedLinear);
	auto pixelLinear = loadedLinear->getPixel(8, 8);
	EXPECT_NEAR(0.214f, pixelLinear.r, 0.02f);
	EXPECT_NEAR(0.214f, pixelLinear.g, 0.02f);
	EXPECT_NEAR(0.214f, pixelLinear.b, 0.02f);
	EXPECT_NEAR(1.0f, pixelLinear.a, 0.01f);

	// 3. Load with sRGB (destination is sRGB)
	auto loadedSRGB = imgManager->loadImage(tempPngPath, false, eImageColorSpace::sRGB);
	ASSERT_NE(nullptr, loadedSRGB);
	auto pixelSRGB = loadedSRGB->getPixel(8, 8);
	EXPECT_NEAR(0.5f, pixelSRGB.r, 0.01f);

	QFile::remove(tempPngPath);
}

TEST(TestUtils, DeferMacro)
{
	int x = 0;
	{
		DEFER(x = 42);
		EXPECT_EQ(0, x);
	}
	EXPECT_EQ(42, x);
}

TEST(TestUtils, DeferMacroMultiple)
{
	int x = 0;
	{
		DEFER(x += 2);
		DEFER(x *= 3); // this will execute before the previous one, so it will multiply first and then add
	}
	EXPECT_EQ(2, x);
}

TEST(TestUtils, ConvertImageAsync)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);

	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	const int w = 64;
	const int h = 64;

	auto src = imgManager->createImage(w, h, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Unknown);
	auto dest = imgManager->createImage(w, h, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::Unknown);
	ASSERT_NE(nullptr, src);
	ASSERT_NE(nullptr, dest);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			src->setPixel(x, y, glm::vec4(static_cast<float>(y) / h, 0.5f, 1.5f, 0.5f));
		}
	}

	// Test flip vertically + clamp (Float -> Byte) + straight to premultiplied alpha
	dest->copyFrom(src, eAlphaProcessing::StraightToPremultiplied);

	// Dest y = 0 corresponds to src y = h - 1 = 63
	auto topPixel = dest->getPixel(0, 0);
	EXPECT_NEAR(topPixel.g, 0.25f, 0.02f);
	EXPECT_NEAR(topPixel.b, 0.75f, 0.02f);
	EXPECT_NEAR(topPixel.a, 0.5f, 0.02f);

	// Test premultiplied to straight alpha
	auto straightDest = imgManager->createImage(w, h, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::Unknown);
	straightDest->copyFrom(dest, eAlphaProcessing::PremultipliedToStraight);
	auto straightPixel = straightDest->getPixel(0, 0);
	EXPECT_NEAR(straightPixel.g, 0.5f, 0.02f);
	EXPECT_NEAR(straightPixel.b, 1.0f, 0.02f);
	EXPECT_NEAR(straightPixel.a, 0.5f, 0.02f);

	// Invalid dimensions check
	auto badDest = imgManager->createImage(32, 32, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::Unknown);
	EXPECT_THROW(badDest->copyFrom(src, eAlphaProcessing::None), std::invalid_argument);
}

TEST(TestUtils, ImageToQImage)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);

	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	const int w = 16;
	const int h = 16;

	// 1. Grayscale byte image -> QImage
	auto grayImg = imgManager->createImage(w, h, eImageFormat::Grayscale, eImageDataType::Byte, eImageColorSpace::sRGB);
	grayImg->setPixel(0, 0, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
	QImage grayQ = grayImg->toQImage();
	EXPECT_EQ(QImage::Format_Grayscale8, grayQ.format());
	EXPECT_EQ(w, grayQ.width());
	EXPECT_EQ(h, grayQ.height());

	// 2. RGB byte image -> QImage
	auto rgbImg = imgManager->createImage(w, h, eImageFormat::RGB, eImageDataType::Byte, eImageColorSpace::sRGB);
	rgbImg->setPixel(0, 0, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	QImage rgbQ = rgbImg->toQImage();
	EXPECT_EQ(QImage::Format_RGB888, rgbQ.format());
	EXPECT_EQ(w, rgbQ.width());
	EXPECT_EQ(h, rgbQ.height());

	// 3. RGBA float image -> QImage with linearToSRGB and alpha straight conversion
	auto rgbaFloatImg = imgManager->createImage(w, h, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Unknown);
	rgbaFloatImg->setPixel(0, 0, glm::vec4(0.25f, 0.5f, 0.25f, 0.5f)); // premultiplied
	QImage rgbaQ = rgbaFloatImg->toQImage(true);
	EXPECT_EQ(QImage::Format_ARGB32, rgbaQ.format());
	EXPECT_EQ(w, rgbaQ.width());
	EXPECT_EQ(h, rgbaQ.height());
	QColor pixel = rgbaQ.pixelColor(0, 0);
	EXPECT_GT(pixel.red(), 0);
	EXPECT_EQ(128, pixel.alpha());
}

TEST(TestUtils, SanitizeFilename)
{
	// 1. Unchanged valid filenames
	EXPECT_EQ(QString("simple_file.txt"), sanitizeFilename("simple_file.txt"));
	EXPECT_EQ(QString("file name with spaces-123.png"), sanitizeFilename("file name with spaces-123.png"));
	EXPECT_EQ(QString("модели-тест 123.obj"), sanitizeFilename("модели-тест 123.obj"));

	// 2. Default replacement ('_') for all forbidden characters: \ / : * ? " < > |
	EXPECT_EQ(QString("a_b_c_d_e_f_g_h_i_j"), sanitizeFilename("a\\b/c:d*e?f\"g<h>i|j"));
	EXPECT_EQ(QString("dir_file.txt"), sanitizeFilename("dir/file.txt"));
	EXPECT_EQ(QString("drive_path"), sanitizeFilename("drive:path"));
	EXPECT_EQ(QString("query__val"), sanitizeFilename("query?*val"));
	EXPECT_EQ(QString("tag___item"), sanitizeFilename("tag<|>item"));
	EXPECT_EQ(QString("_quoted_"), sanitizeFilename("\"quoted\""));

	// 3. Custom replacement string
	EXPECT_EQ(QString("dir-file.txt"), sanitizeFilename("dir/file.txt", "-"));
	EXPECT_EQ(QString("a#b#c"), sanitizeFilename("a/b:c", "#"));
	EXPECT_EQ(QString("stripped"), sanitizeFilename("st/ri:pp*ed", ""));

	// 4. Consecutive forbidden characters
	EXPECT_EQ(QString("file___name.ext"), sanitizeFilename("file/?:name.ext"));
	EXPECT_EQ(QString("file---name.ext"), sanitizeFilename("file/?:name.ext", "-"));

	// 5. Edge cases: empty string and string of only forbidden characters
	EXPECT_EQ(QString(""), sanitizeFilename(""));
	EXPECT_EQ(QString("_________"), sanitizeFilename("\\/:*?\"<>|"));
	EXPECT_EQ(QString(""), sanitizeFilename("\\/:*?\"<>|", ""));
}

TEST(TestUtils, RelativePath)
{
	// 1. Files inside directory (direct child and nested subdirectories)
	EXPECT_EQ(QString("diffuse.png"), relativePath("/Users/someone/Project/diffuse.png", "/Users/someone/Project"));
	EXPECT_EQ(QString("textures/wood.png"), relativePath("/Users/someone/Project/textures/wood.png", "/Users/someone/Project"));
	EXPECT_EQ(QString("sub/textures/wood.png"), relativePath("/Users/someone/Project/sub/textures/wood.png", "/Users/someone/Project"));

	// 2. Trailing slash in base directory
	EXPECT_EQ(QString("textures/wood.png"), relativePath("/Users/someone/Project/textures/wood.png", "/Users/someone/Project/"));

	// 3. Redundant slashes and relative segments (. and ..)
	EXPECT_EQ(QString("textures/wood.png"), relativePath("/Users/someone/Project/sub/../textures/wood.png", "/Users/someone/Project/./"));

	// 4. Sibling directory with shared prefix name must not match as inside
	EXPECT_EQ(QString("/Users/someone/Project_Backup/file.png"), relativePath("/Users/someone/Project_Backup/file.png", "/Users/someone/Project"));

	// 5. Parent / outside directory remains absolute
	EXPECT_EQ(QString("/Users/someone/Textures/wood.png"), relativePath("/Users/someone/Textures/wood.png", "/Users/someone/Project"));
	EXPECT_EQ(QString("/Library/Textures/wood.png"), relativePath("/Library/Textures/wood.png", "/Users/someone/Project"));

	// 6. Same directory
	EXPECT_EQ(QString("."), relativePath("/Users/someone/Project", "/Users/someone/Project"));

	// 7. Root directory
	EXPECT_EQ(QString("file.png"), relativePath("/file.png", "/"));

#ifdef _WIN32
	// 8. Windows paths (Qt returns forward slashes, case-insensitivity supported by Windows QDir)
	EXPECT_EQ(QString("textures/wood.png"), relativePath("C:\\Projects\\Scene\\textures\\wood.png", "C:\\Projects\\Scene"));
	EXPECT_EQ(QString("textures/wood.png"), relativePath("C:/Projects/Scene/textures/wood.png", "C:/Projects/Scene"));
	EXPECT_EQ(QString("Textures/Wood.png"), relativePath("c:\\projects\\scene\\Textures\\Wood.png", "C:\\Projects\\Scene"));
	EXPECT_EQ(QString("D:\\Assets\\wood.png"), relativePath("D:\\Assets\\wood.png", "C:\\Projects\\Scene"));
#endif

	// 9. Empty inputs
	EXPECT_EQ(QString("/path/to/file.png"), relativePath("/path/to/file.png", ""));
	EXPECT_EQ(QString(""), relativePath("", "/path/to/dir"));
	EXPECT_EQ(QString(""), relativePath("", ""));

	// 10. Qt Resources and URI schemes
	EXPECT_EQ(QString(":/textures/wood.png"), relativePath(":/textures/wood.png", "/Users/someone/Project"));
	EXPECT_EQ(QString("miray:///Library/Textures/wood.png"), relativePath("miray:///Library/Textures/wood.png", "/Users/someone/Project"));
}

TEST(TestUtils, AbsolutePath)
{
	// 1. Relative file path + base directory
	EXPECT_EQ(nativePath("/Users/someone/Project/textures/wood.png"), absolutePath("textures/wood.png", "/Users/someone/Project"));
	EXPECT_EQ(nativePath("/Users/someone/Project/diffuse.png"), absolutePath("diffuse.png", "/Users/someone/Project"));

	// 2. Already absolute file path (must not prepend dir)
	EXPECT_EQ(nativePath("/Library/Textures/wood.png"), absolutePath("/Library/Textures/wood.png", "/Users/someone/Project"));

	// 3. Empty inputs
	EXPECT_EQ(QString(""), absolutePath("", "/Users/someone/Project"));
	EXPECT_EQ(nativePath("textures/wood.png"), absolutePath("textures/wood.png", ""));
	EXPECT_EQ(QString(""), absolutePath("", ""));

	// 4. Resources and URI schemes
	EXPECT_EQ(nativePath(":/textures/wood.png"), absolutePath(":/textures/wood.png", "/Users/someone/Project"));
	EXPECT_EQ(nativePath("miray:///Library/Textures/wood.png"), absolutePath("miray:///Library/Textures/wood.png", "/Users/someone/Project"));
}

TEST(TestUtils, FindExistingFileAndResolveFilePath)
{
	QString tempDir = QDir::tempPath();
	QString tempFileName = "miray_test_file_utils_tmp.txt";
	QString tempFilePath = QDir(tempDir).filePath(tempFileName);

	writeFile(tempFilePath, "test");
	DEFER(QFile::remove(tempFilePath));

	// 1. findExistingFile finds existing file directly in dir
	QString found = findExistingFile(tempFileName, tempDir);
	EXPECT_FALSE(found.isEmpty());
	EXPECT_EQ(nativePath(QFileInfo(tempFilePath).canonicalFilePath()), found);

	// 2. findExistingFile finds relocated file when path had old subfolder/prefix
	QString foundRelocated = findExistingFile("old_missing_folder/" + tempFileName, tempDir);
	EXPECT_EQ(nativePath(QFileInfo(tempFilePath).canonicalFilePath()), foundRelocated);

	// 3. findExistingFile returns empty string if file does not exist
	EXPECT_TRUE(findExistingFile("non_existent_file_12345.xyz", tempDir).isEmpty());
	EXPECT_TRUE(findExistingFile("", tempDir).isEmpty());

	// 4. resolveFilePath returns existing file
	EXPECT_EQ(nativePath(QFileInfo(tempFilePath).canonicalFilePath()), resolveFilePath(tempFileName, tempDir));

	// 5. resolveFilePath falls back to absolutePath if file does not exist
	EXPECT_EQ(nativePath(QDir(tempDir).filePath("non_existent_file_12345.xyz")), resolveFilePath("non_existent_file_12345.xyz", tempDir));
	EXPECT_EQ(QString(""), resolveFilePath("", tempDir));
}

TEST(TestUtils, ResourcePathHandling)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);

	// 1. miray:// should NOT be converted to a global path
	EXPECT_EQ("miray://Library/Textures/metal.png", app.getShortResourcePath("miray://Library/Textures/metal.png"));
	EXPECT_EQ("miray://Library/Textures/metal.png", app.getShortResourcePath("miray:///Library/Textures/metal.png"));
	EXPECT_EQ("miray://Library/Textures/metal.png", app.getShortResourcePath("miray:\\\\Library\\Textures\\metal.png"));
	EXPECT_EQ("miray://Library/Textures/metal.png", app.getShortResourcePath("owlet://Library/Textures/metal.png"));
	EXPECT_EQ("miray://Library/Textures/metal.png", app.getShortResourcePath("owlet:///Library/Textures/metal.png"));

	// 2. Paths inside Resources folder must convert to miray://
	const auto resPath = app.getResourcesFolder().absoluteFilePath("Library/Textures/metal.png");
	EXPECT_EQ("miray://Library/Textures/metal.png", app.getShortResourcePath(resPath));

	// 3. file:// pointing to Resources folder must convert to miray://
	EXPECT_EQ("miray://Library/Textures/metal.png", app.getShortResourcePath("file://" + resPath));

	// 4. External path should NOT be converted to miray://
	const auto externalPath = "/Users/someone/photos/test.png";
	EXPECT_EQ(externalPath, app.getShortResourcePath(externalPath));

	// 5. isInternalResource checks
	EXPECT_TRUE(app.isInternalResource("miray://Library/Textures/metal.png"));
	EXPECT_TRUE(app.isInternalResource("owlet://Library/Textures/metal.png"));
	EXPECT_TRUE(app.isInternalResource(resPath));
	EXPECT_TRUE(app.isInternalResource("file://" + resPath));
	EXPECT_FALSE(app.isInternalResource(externalPath));
}

inline QString urlToLocalFile(const char * str) { return urlToLocalFile(QString::fromUtf8(str)); }

TEST(TestUtils, UrlToLocalFile)
{
	// 1. file:// QString
	EXPECT_EQ("/path/to/photo.png", urlToLocalFile("file:///path/to/photo.png"));

	// 2. Regular string without file://
	EXPECT_EQ("/path/to/photo.png", urlToLocalFile("/path/to/photo.png"));
	EXPECT_EQ("miray://Library/Textures/wood.png", urlToLocalFile("miray://Library/Textures/wood.png"));

	// 3. QVariant containing QUrl with file://
	EXPECT_EQ("/path/to/photo.png", urlToLocalFile(QVariant::fromValue(QUrl("file:///path/to/photo.png"))));

	// 4. QVariant containing QString with file://
	EXPECT_EQ("/path/to/photo.png", urlToLocalFile(QVariant("file:///path/to/photo.png")));

	// 5. QVariant containing QString with miray://
	EXPECT_EQ("miray://Library/Textures/wood.png", urlToLocalFile(QVariant("miray://Library/Textures/wood.png")));

	// 6. QVariant containing custom scheme QUrl (QUrl normalizes host to lowercase per RFC 3986)
	EXPECT_EQ("miray://library/Textures/wood.png", urlToLocalFile(QVariant::fromValue(QUrl("miray://Library/Textures/wood.png"))));
}

TEST(TestUtils, CheckIfImageIsNormalMap)
{
	int argc = 0;
	char **argv = nullptr;
	Application app(argc, argv);
	app.init(eLoggingMode::None);
	auto imgManager = app.imageManager();
	ASSERT_NE(nullptr, imgManager);

	// 1. Null image
	EXPECT_FALSE(checkIfImageIsNormalMap(nullptr));

	// 2. Grayscale image
	auto grayImg = imgManager->createImage(16, 16, eImageFormat::Grayscale, eImageDataType::Byte, eImageColorSpace::Linear);
	for (int y = 0; y < 16; ++y)
		for (int x = 0; x < 16; ++x)
			grayImg->setPixel(x, y, vec4(0.5f, 0.5f, 0.5f, 1.f));
	EXPECT_FALSE(checkIfImageIsNormalMap(grayImg));

	// 3. Flat normal map (0.5, 0.5, 1.0)
	auto normalImg = imgManager->createImage(16, 16, eImageFormat::RGB, eImageDataType::Byte, eImageColorSpace::Linear);
	for (int y = 0; y < 16; ++y)
		for (int x = 0; x < 16; ++x)
			normalImg->setPixel(x, y, vec4(0.5f, 0.5f, 1.0f, 1.f));
	EXPECT_TRUE(checkIfImageIsNormalMap(normalImg));

	// 4. Gray height map in RGB format (0.5, 0.5, 0.5)
	auto grayRgbImg = imgManager->createImage(16, 16, eImageFormat::RGB, eImageDataType::Byte, eImageColorSpace::Linear);
	for (int y = 0; y < 16; ++y)
		for (int x = 0; x < 16; ++x)
			grayRgbImg->setPixel(x, y, vec4(0.5f, 0.5f, 0.5f, 1.f));
	EXPECT_FALSE(checkIfImageIsNormalMap(grayRgbImg));

	// 5. White height map in RGB format (1.0, 1.0, 1.0)
	auto whiteImg = imgManager->createImage(16, 16, eImageFormat::RGB, eImageDataType::Byte, eImageColorSpace::Linear);
	for (int y = 0; y < 16; ++y)
		for (int x = 0; x < 16; ++x)
			whiteImg->setPixel(x, y, vec4(1.0f, 1.0f, 1.0f, 1.f));
	EXPECT_FALSE(checkIfImageIsNormalMap(whiteImg));

	// 6. Color texture (red / green dominant)
	auto redImg = imgManager->createImage(16, 16, eImageFormat::RGB, eImageDataType::Byte, eImageColorSpace::Linear);
	for (int y = 0; y < 16; ++y)
		for (int x = 0; x < 16; ++x)
			redImg->setPixel(x, y, vec4(0.8f, 0.2f, 0.1f, 1.f));
	EXPECT_FALSE(checkIfImageIsNormalMap(redImg));

	// 7. Bumpy normal map with normalized perturbation
	auto bumpyNormalImg = imgManager->createImage(16, 16, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::Linear);
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			float nx = ((x % 3) - 1) * 0.2f;
			float ny = ((y % 3) - 1) * 0.2f;
			float nz = std::sqrt(std::max(0.0f, 1.0f - nx * nx - ny * ny));
			bumpyNormalImg->setPixel(x, y, vec4(nx * 0.5f + 0.5f, ny * 0.5f + 0.5f, nz * 0.5f + 0.5f, 1.f));
		}
	}
	EXPECT_TRUE(checkIfImageIsNormalMap(bumpyNormalImg));
}
