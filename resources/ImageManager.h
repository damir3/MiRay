//
//  ImageManager.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 08.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "Image.h"

namespace mr
{

class ImageManager : public IResourceManager
{
public:
	ImageManager();
	~ImageManager();

	Image * Create(int w, int h, Image::eType t);
	Image * Load(const char * strFilename);
	Image * CreateCopy(const Image * pSrcImage, Image::eType t);
	Image * LoadNormalmap(const char * strFilename);

	enum eFileFormat
	{
		FILE_FORMAT_AUTO,
		FILE_FORMAT_PNG,
		FILE_FORMAT_JPEG,
		FILE_FORMAT_TIFF,
		FILE_FORMAT_TARGA,
		FILE_FORMAT_DDS,
	};

	bool Save(const char * strFilename, const Image & image, eFileFormat ff = FILE_FORMAT_AUTO);
};

}