//
//  Image.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 03.02.14.
//  Copyright (c) 2014 Damir Sagidullin. All rights reserved.
//

namespace mr
{

class IImage
{
public:
	virtual ~IImage() {}

	virtual int Width() const = 0;
	virtual int Height() const = 0;
	virtual const void * Data() const = 0;
	virtual void * Data() = 0;

	virtual void SetPixel(int x, int y, const ColorF & c) = 0;
	virtual ColorF GetPixel(int x, int y) const = 0;
	virtual ColorF GetPixel(float u, float v) const = 0;
};

}