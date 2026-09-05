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

#pragma once

#include "ParametersImpl.h"

struct Ray;

class CORE_EXPORT TextureImpl final : public ITexture, public IParameterOwner
{
	Q_OBJECT

	IParameterOwner &		m_owner;
	const eParamId			m_id;
	const eType				m_type;

	BooleanParameterImpl	m_enabled;
	FileNameParameterImpl	m_fileName;
	BooleanParameterImpl	m_normapMap;
	BooleanParameterImpl	m_invert;
	EnumParameterImpl		m_mapping;
	Vec2ParameterImpl		m_repeat;
	Vec2ParameterImpl		m_offset;
	ScalarParameterImpl		m_rotation;
	ScalarParameterImpl		m_cropLeft;
	ScalarParameterImpl		m_cropTop;
	ScalarParameterImpl		m_cropRight;
	ScalarParameterImpl		m_cropBottom;
	EnumParameterImpl		m_wrapX;
	EnumParameterImpl		m_wrapY;

	ScalarParameterImpl		m_brightness;
	ScalarParameterImpl		m_contrast;
	ScalarParameterImpl		m_gamma;

	ImagePtr	m_originImage;
	ImagePtr	m_image;
	void updateImage(bool fileNameChanged = false);
	void loadImage(bool fileNameChanged = false);
	bool allocateImage(int width, int height, eImageFormat format, eImageDataType dataType, eImageColorSpace colorSpace);

	bool	m_isTransparent;
	bool	m_hasTransofmation;
	bool	m_hasCrop;
	RectF	m_crop;
	vec3	m_tm[2];
	void updateTransformation();

	int getFlags() const;

private slots:
	void onImageUpdated(const IImage *);

public:
	TextureImpl(IParameterOwner & owner, eParamId paramId, eType type);
	~TextureImpl() override;

	static QString safeGetFileName(const ITexture *t);

	void _set(bool enabled, const QString &fileName, const RectF &crop = RectF(0.f, 0.f, 1.f, 1.f),
					 eWrap wrapX = WRAP_REPEAT, eWrap wrapY = WRAP_REPEAT, eMapping mapping = MAPPING_UV0,
					 const vec2 &repeat = vec2(1.f), const vec2 &offset = vec2(0.f), float rotation = 0.f,
					 bool invert = false, float brightness = 0.f, float contrast = 0.f, float gamma = 1.f,
					 bool normalMap = false) override;
	void reset(const QString &fileName)
	{
		_set(true, fileName, RectF(0.f, 0.f, 1.f, 1.f), WRAP_REPEAT, WRAP_REPEAT, MAPPING_UV0,
					vec2(1.f), vec2(0.f), 0.f, false, 0.f, 0.f, 1.f, false);
	}

	bool isEmpty() const final override { return m_image.get() == nullptr; }
	IImage * getImage() const { return m_enabled ? m_image.get() : nullptr; }
	ImagePtr getOriginalImagePtr() const { return m_originImage; }

	BooleanParameterImpl & enabled() final override { return m_enabled; }

	FileNameParameterImpl & fileName() override { return m_fileName; }
	const FileNameParameterImpl & fileName() const override { return m_fileName; }

	BooleanParameterImpl & normalMap() override { return m_normapMap; }

	BooleanParameterImpl & invert() override { return m_invert; }

	EnumParameterImpl & mapping() override { return m_mapping; }

	Vec2ParameterImpl & repeat() override { return m_repeat; }

	Vec2ParameterImpl & offset() override { return m_offset; }

	ScalarParameterImpl & rotation() override { return m_rotation; }

	ScalarParameterImpl & cropLeft() override { return m_cropLeft; }
	ScalarParameterImpl & cropTop() override { return m_cropTop; }
	ScalarParameterImpl & cropRight() override { return m_cropRight; }
	ScalarParameterImpl & cropBottom() override { return m_cropBottom; }

	EnumParameterImpl & wrapX() override { return m_wrapX; }
	EnumParameterImpl & wrapY() override { return m_wrapY; }

	ScalarParameterImpl & brightness() override { return m_brightness; }
	ScalarParameterImpl & contrast() override { return m_contrast; }
	ScalarParameterImpl & gamma() override { return m_gamma; }

	bool valid() const { return m_enabled && m_image.get() != nullptr; }
	bool isTransparent() const { return valid() && m_isTransparent; }

	inline static void repeatF(float & f)
	{
		if (f > 1.f)
			f = fmodf(f, 1.f);
		else if (f < 0.f)
			f = fmodf(f, 1.f) + 1.f;
	}

	inline static void clampF(float & f)
	{
		if (f > 1.f)
			f = 1.f;
		else if (f < 0.f)
			f = 0.f;
	}

	inline static void mirrorF(float & f)
	{
		if (f > 1.f)
			f = 1.f - fmodf(f, 1.f);
		else if (f < 0.f)
			f = fmodf(-f, 1.f);
	}

	vec2 getTC(const Ray & ray) const;
//	void getTriangleTC(vec2 & tc0, vec2 & tc1, vec2 & tc2, const Ray & ray) const;
	void getTangentBinormal(vec3 & tangent, vec3 & binormal, const Ray & ray, float bumpDepth) const;
	vec3 getTangentDir(const Ray & ray) const;

	vec2 getUV(const vec2 & tc) const
	{
		vec2 uv;
		if (m_hasTransofmation) {
			uv.x = tc.x * m_tm[0].x + tc.y * m_tm[0].y + m_tm[0].z;
			uv.y = tc.x * m_tm[1].x + tc.y * m_tm[1].y + m_tm[1].z;
		} else
			uv = tc;

		switch (m_wrapX.getIndex()) {
			case WRAP_REPEAT:	repeatF(uv.x); break;
			case WRAP_CLAMP:	clampF(uv.x); break;
			case WRAP_MIRROR:	mirrorF(uv.x); break;
		}

		switch (m_wrapY.getIndex()) {
			case WRAP_REPEAT:	repeatF(uv.y); break;
			case WRAP_CLAMP:	clampF(uv.y); break;
			case WRAP_MIRROR:	mirrorF(uv.y); break;
		}

		if (m_hasCrop) {
			uv.x = lerp(m_crop.left, m_crop.right, uv.x);
			uv.y = lerp(m_crop.top, m_crop.bottom, uv.y);
		}

		return uv;
	}

	vec2 getUV(const Ray & ray) const
	{
		return getUV(getTC(ray));
	}

	float getScalar(const Ray & ray) const
	{
		auto uv = getUV(ray);
		auto value = m_image->getPixelColorUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT).x;
		return m_invert ? 1.f - value : value;
	}

	vec3 getColor(const vec2 & tc) const
	{
		auto uv = getUV(tc);
		auto color = m_image->getPixelColorUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT);
		return m_invert ? vec3(1.f) - color : color;
	}

	vec3 getColor(const Ray & ray) const
	{
		auto uv = getUV(ray);
		auto color = m_image->getPixelColorUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT);
		return m_invert ? vec3(1.f) - color : color;
	}

	vec4 getColor4(const vec2 & tc) const
	{
		auto uv = getUV(tc);
		auto color = m_image->getPixelUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT);
		if (m_invert) {
			color.r = 1.f - color.r;
			color.g = 1.f - color.g;
			color.b = 1.f - color.b;
		}
		return color;
	}

	vec4 getColor4(const Ray & ray) const
	{
		auto uv = getUV(ray);
		auto color = m_image->getPixelUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT);
		if (m_invert) {
			color.r = 1.f - color.r;
			color.g = 1.f - color.g;
			color.b = 1.f - color.b;
		}
		return color;
	}

	float getOpacity(const Ray & ray) const
	{
		if (!m_isTransparent)
			return 1.f;

		auto uv = getUV(ray);
		return m_image->getPixelOpacityUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT);
	}

	vec3 getNormal(const vec2 & tc) const
	{
		auto uv = getUV(tc);
		auto nm = m_image->getPixelColorUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT) * 2.f - 1.f;

		if (m_invert) {
			nm.x = -nm.x;
			nm.y = -nm.y;
		}

		if (m_hasCrop) {
			nm.x *= m_crop.right - m_crop.left;
			nm.y *= m_crop.bottom - m_crop.top;
		}

		return m_hasTransofmation ? vec3(m_tm[0].x * nm.x + m_tm[1].x * nm.y, m_tm[0].y * nm.x + m_tm[1].y * nm.y, nm.z) : nm;
	}

	float getHeight(const vec2 & tc) const
	{
		auto uv = getUV(tc);
		auto height = m_image->getPixelOpacityUV(uv.x, uv.y, m_wrapX.getIndex() != WRAP_REPEAT, m_wrapY.getIndex() != WRAP_REPEAT);
		return glm::clamp(m_invert ? 1.f - height : height, 0.f, 1.f);
	}

	bool isDefault() const;
	bool _load(const QDomElement & node, const ModelLoadingContext &ctx);
	bool _load(const QJsonObject & obj, const ModelLoadingContext &ctx);
	void _save(QJsonObject & obj, const ModelSavingContext &ctx) const;

	bool load(const QByteArray & data, const ModelLoadingContext &ctx) override;
	QByteArray save(const ModelSavingContext &ctx) const override;

	QJsonObject getState() const;
	void setState(const QJsonObject & state);

	bool isEqual(const TextureImpl & other) const;
	void copyFrom(const TextureImpl &srcTexture);

	struct TransitionState {
		ImagePtr image;
		bool	isTransparent;
		bool	hasTransofmation;
		bool	hasCrop;
		RectF	crop;
		vec3	tm[2];
		int		wrap[2];
		bool	invert;
	};

	void getTransitionState(TransitionState & state) const;
	void setTransitionState(const TransitionState & state);

	void setEnabledMappingParams(bool enable);

	QImage getPreview() const override;

	CoreInstance & core() const override { return m_owner.core(); }
	void pushCommand(QUndoCommand *cmd) override { m_owner.pushCommand(cmd); }
	void fireChanged(eParamId paramId) override;

	static void updateFileNames(const QMap<QString, QString>&, ITexture*);
};
