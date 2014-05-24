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

typedef std::shared_ptr<Image> ImagePtr;

class ImageManager
{
	std::map<std::string, std::weak_ptr<Image>> m_resources;

	friend Image;
	void Release(const std::string & name);

public:
	ImageManager();
	~ImageManager();

	ImagePtr Create(int w, int h, Image::eType t);
	ImagePtr Load(const char * strFilename);
	ImagePtr CreateCopy(const Image * pSrcImage, Image::eType t);
	ImagePtr LoadNormalmap(const char * strFilename);

	enum eFileFormat
	{
		FILE_FORMAT_AUTO,
		FILE_FORMAT_PNG,
		FILE_FORMAT_JPEG,
		FILE_FORMAT_TIFF,
		FILE_FORMAT_TARGA,
		FILE_FORMAT_DDS,
	};

	bool Save(const char * strFilename, const Image & image, bool saveAlpha, eFileFormat ff = FILE_FORMAT_AUTO);
};

}