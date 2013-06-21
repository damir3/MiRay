//
//  ImageManager.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 08.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "ImageManager.h"
#include "../ThirdParty/FreeImage/FreeImage.h"

using namespace mr;

// ------------------------------------------------------------------------ //

ImageManager::ImageManager()
{
}

ImageManager::~ImageManager()
{
}

// ------------------------------------------------------------------------ //

Image * ImageManager::Create(int w, int h, Image::eType t)
{
	Image * pImage = new Image(*this, "");
	if (!pImage)
		return NULL;

	pImage->Create(w, h, t);

	return pImage;
}

Image * ImageManager::CreateCopy(const Image * pSrcImage, Image::eType t)
{
	if (!pSrcImage)
		return NULL;

	Image * pImage = new Image(*this, "");
	if (!pImage)
		return NULL;

	const int w = pSrcImage->Width();
	const int h = pSrcImage->Height();
	pImage->Create(w, h, t);
	if (!pImage->Data())
	{
		SAFE_RELEASE(pImage);
		return NULL;
	}

	if (pSrcImage->Type() == pImage->Type())
	{
		memcpy(pImage->Data(), pSrcImage->Data(), w * h * pImage->PixelSize());
	}
	else
	{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
				pImage->SetPixel(x, y, pSrcImage->GetPixel(x, y));
		}
	}
	
	return pImage;
}

// ------------------------------------------------------------------------ //

//bool ReadFile(std::vector<byte> & data, const char * strFilename)
//{
//	FILE * f = fopen(strFilename, "rb");
//	if (!f)
//		return false;
//
//	fseek(f, 0, SEEK_END);
//	size_t fileSize = ftell(f);
//	fseek(f, 0, SEEK_SET);
//
//	data.resize(fileSize);
//	size_t readSize = fread(data.data(), 1, fileSize, f);
//
//	fclose(f);
//
//	return !data.empty() && (readSize == fileSize);
//}
//
//bool SaveFile(const char * strFilename, const std::vector<byte> & buf)
//{
//	FILE * f = fopen(strFilename, "wb");
//	if (!f)
//		return false;
//	
//	size_t writeSize = fwrite(buf.data(), buf.size(), 1, f);
//	
//	fclose(f);
//	
//	return writeSize == buf.size();
//}

// ------------------------------------------------------------------------ //

Image * ImageManager::Load(const char * strFilename)
{
	if (!strFilename || !*strFilename)
		return NULL;

	Image * pImage = static_cast<Image *>(Find(strFilename));
	if (pImage)
	{
		pImage->AddRef();
		return pImage;
	}
	
	printf("Loading image '%s'...\n", strFilename);
	
	//check the file signature and deduce its format
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(strFilename, 0);
	if (fif == FIF_UNKNOWN) // if still unknown, try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(strFilename);

	if (fif == FIF_UNKNOWN)
		return NULL; // if still unkown, return failure

	//check that the plugin has reading capabilities and load the file
	FIBITMAP * dib = NULL;
	if (FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, strFilename);

	if (!dib)
		return NULL; // if the image failed to load, return failure

	uint32 width = FreeImage_GetWidth(dib);
	uint32 height = FreeImage_GetHeight(dib);
	uint32 bpp = FreeImage_GetBPP(dib);
	if ((FreeImage_GetBits(dib) == NULL) || (width == 0) || (height == 0) || (bpp == 0))
	{// if this somehow one of these failed (they shouldn't), return failure
		FreeImage_Unload(dib);
		return NULL;
	}

//	unsigned dpmX = FreeImage_GetDotsPerMeterX(dib);
//	unsigned dpmY = FreeImage_GetDotsPerMeterY(dib);
	FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(dib);
	FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(dib);
	if ((colorType == FIC_RGB) || (colorType == FIC_RGBALPHA))
	{
		Image::eType it = Image::TYPE_NONE;
		switch (imageType)
		{
			case FIT_BITMAP:
				if (bpp == 24)
					it = Image::TYPE_3B;
				else if (bpp == 32)
					it = Image::TYPE_4B;
				break;
			case FIT_RGB16: it = Image::TYPE_3W; break;
			case FIT_RGBA16: it = Image::TYPE_4W; break;
			case FIT_RGBF: it = Image::TYPE_3F; break;
			case FIT_RGBAF: it = Image::TYPE_4F; break;
			default: break;
		}

		if (it == Image::TYPE_NONE)
		{
			it = Image::TYPE_3B;
			FIBITMAP * dibNew = FreeImage_ConvertTo24Bits(dib);
			FreeImage_Unload(dib);
			dib = dibNew;
		}

		byte * bits = FreeImage_GetBits(dib);
		if (bits != NULL)
		{
			pImage = new Image(*this, strFilename);
			if (pImage->Create(width, height, it))
			{
				uint32 srcPitch = FreeImage_GetPitch(dib);
				uint32 destPixelSize = pImage->PixelSize();
				uint32 destPitch = static_cast<uint32>(pImage->Width() * destPixelSize);
				uint32 pitch = std::min(srcPitch, destPitch);
				for (uint32 y = 0 ; y < height; y++)
					memcpy(pImage->Data() + y * destPitch, bits + (height - y - 1) * srcPitch, pitch);

				if ((FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR) && (bpp == 24 || bpp == 32))
				{
					for (uint32 y = 0 ; y < height; y++)
					{
						byte * pDest = pImage->Data() + y * destPitch;
						for (uint32 x = 0; x < width; x++)
						{
							std::swap(pDest[0], pDest[2]);
							pDest += destPixelSize;
						}
					}
				}
			}
		}
	}
	else if (colorType == FIC_PALETTE)
	{
	}

	FreeImage_Unload(dib);

	return pImage;
}

// ------------------------------------------------------------------------ //

bool ImageManager::Save(const char * strFilename, const Image & image, eFileFormat ff)
{
	FREE_IMAGE_FORMAT fif;
	switch (ff)
	{
		default:
		case FILE_FORMAT_AUTO: fif = FreeImage_GetFIFFromFilename(strFilename); break;
		case FILE_FORMAT_PNG: fif = FIF_PNG; break;
		case FILE_FORMAT_JPEG: fif = FIF_JPEG; break;
		case FILE_FORMAT_TIFF: fif = FIF_TIFF; break;
		case FILE_FORMAT_TARGA: fif = FIF_TARGA; break;
		case FILE_FORMAT_DDS: fif = FIF_DDS; break;
	}

	int width = image.Width();
	int height = image.Height();
	int bpp = image.PixelSize();
	const byte * pSrcData = image.Data();

	FREE_IMAGE_TYPE type;
	switch (image.Type())
	{
		default:
		case Image::TYPE_3B:
		case Image::TYPE_4B:
			type = FIT_BITMAP;
			break;
		case Image::TYPE_3W: type = FIT_RGB16; break;
		case Image::TYPE_4W: type = FIT_RGBA16; break;
		case Image::TYPE_3F: type = FIT_RGBF; break;
		case Image::TYPE_4F: type = FIT_RGBAF; break;
	}

	Image * pTmpImage = NULL;
	if (!FreeImage_FIFSupportsExportBPP(fif, bpp * 8) ||
		!FreeImage_FIFSupportsExportType(fif, type))
	{
		if (!FreeImage_FIFSupportsExportType(fif, FIT_BITMAP))
			return false;

		type = FIT_BITMAP;
		if (FreeImage_FIFSupportsExportBPP(fif, 32))
			bpp = 4;
		else if (FreeImage_FIFSupportsExportBPP(fif, 24))
			bpp = 3;
		else
			return false;

		pTmpImage = CreateCopy(&image, bpp == 4 ? Image::TYPE_4B : Image::TYPE_3B);
		if (!pTmpImage)
			return false;

		pSrcData = pTmpImage->Data();
	}

	bool res = false;

	FIBITMAP * dib = FreeImage_AllocateT(type, width, height, bpp * 8);
	if (dib != NULL)
	{
		byte * bits = FreeImage_GetBits(dib);
		if (bits != NULL)
		{
			uint32 destPitch = FreeImage_GetPitch(dib);
			uint32 srcPitch = static_cast<uint32>(width * bpp);
			uint32 pitch = std::min(srcPitch, destPitch);
			for (int y = 0 ; y < height; y++)
				memcpy(bits + (height - y - 1) * destPitch, pSrcData + y * srcPitch, pitch);

			if ((FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR) && (bpp == 3 || bpp == 4))
			{
				for (int y = 0 ; y < height; y++)
				{
					byte * pDest = bits + y * destPitch;
					for (int x = 0; x < width; x++)
					{
						std::swap(pDest[0], pDest[2]);
						pDest += bpp;
					}
				}
			}
			
			res = FreeImage_Save(fif, dib, strFilename) != FALSE;
		}

		FreeImage_Unload(dib);
	}

	if (pTmpImage)
		pTmpImage->Release();

	return res;
}

Image * ImageManager::LoadNormalmap(const char * strFilename)
{
	if (!strFilename || !*strFilename)
		return NULL;

	std::string strNormalmapName = std::string("normalmap@") + strFilename;
	Image * pNormalmapImage = static_cast<Image *>(Find(strNormalmapName.c_str()));
	if (pNormalmapImage)
	{
		pNormalmapImage->AddRef();
		return pNormalmapImage;
	}

	Image * pImage = Load(strFilename);
	if (!pImage)
		return NULL;

	pNormalmapImage = new Image(*this, strNormalmapName.c_str());
	if (pNormalmapImage)
	{
		const int w = pImage->Width();
		const int h = pImage->Height();
		pNormalmapImage->Create(w, h, Image::TYPE_4F);

		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				Vec3 normal;
				normal.x  = pImage->GetPixel(x > 0 ? x - 1 : w - 1, y).r;
				normal.x -= pImage->GetPixel(x + 1 < w ? x + 1 : 0, y).r;
				normal.y  = pImage->GetPixel(x, y > 0 ? y - 1 : h - 1).r;
				normal.y -= pImage->GetPixel(x, y + 1 < h ? y + 1 : 0).r;
				normal.z = 0.25f;
				normal.Normalize();
				normal = normal * 0.5f + Vec3(0.5f, 0.5f, 0.5f);
				pNormalmapImage->SetPixel(x, y, ColorF(normal.x, normal.y, normal.z, pImage->GetPixel(x, y).r));
			}
		}
	}

	pImage->Release();

	return pNormalmapImage;
}
