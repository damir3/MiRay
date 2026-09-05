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

#include "TextureImpl.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "../../Shared/Interfaces/SerializationContext.h"
#include "ImageManager.h"
#include "Utils.h"
#include "Materials/MaterialManager.h"

QString TextureImpl::safeGetFileName(const ITexture *tex)
{
	return tex ? tex->fileName().get() : QString();
}

static const char * const WRAP_MODES[] = {"Repeat", "Clamp", "Mirror", nullptr};
static const char * const MAPPING_TYPES[] = {
	"UV0", "UV1", "UV2", "UV3",
//	"Box", "Planar X", "Planar Y", "Planar Z",
//	"Cylindrical X", "Cylindrical Y", "Cylindrical Z",
//	"Spherical X", "Spherical Y", "Spherical Z",
	nullptr};

#define CONTRAST_COEF		(259.f / 255.f)

FileFormatsList convertImageManagerFormatsToFilePicker(const ImageLoadingFormats& fmts)
{
	FileFormatsList exts;

	foreach(const auto& fmt, fmts) {
		FileFormat p = { fmt.fileType, fmt.extensions };
		exts.push_back(p);
	}

	return exts;
}

TextureImpl::TextureImpl(IParameterOwner & owner, eParamId paramId, eType type)
	: m_owner(owner)
	, m_id(paramId)
	, m_type(type)
	, m_enabled(true, PID_TEXTURE_ENABLED, *this)
	, m_fileName(convertImageManagerFormatsToFilePicker(m_owner.core().appContext().imageManager()->loadingFormats()),
		PID_TEXTURE_FILENAME, *this)
	, m_mapping(MAPPING_UV0, MAPPING_TYPES, PID_TEXTURE_MAPPING, *this)
	, m_repeat(vec2(1.f, 1.f), 3, PID_TEXTURE_REPEAT, *this)
	, m_offset(vec2(0.f, 0.f), 3, PID_TEXTURE_OFFSET, *this)
	, m_rotation(0.f, -360.f, 360.f, 1, PID_TEXTURE_ROTATION, *this)
	, m_cropLeft(0.f, 0.f, 1.f, 3, PID_TEXTURE_CROP, *this)
	, m_cropTop(0.f, 0.f, 1.f, 3, PID_TEXTURE_CROP, *this)
	, m_cropRight(1.f, 0.f, 1.f, 3, PID_TEXTURE_CROP, *this)
	, m_cropBottom(1.f, 0.f, 1.f, 3, PID_TEXTURE_CROP, *this)
	, m_wrapX(WRAP_REPEAT, WRAP_MODES, PID_TEXTURE_WRAP_X, *this)
	, m_wrapY(WRAP_REPEAT, WRAP_MODES, PID_TEXTURE_WRAP_Y, *this)
	, m_normapMap(false, PID_TEXTURE_NORMAL_MAP, *this)
	, m_invert(false, PID_TEXTURE_INVERT, *this)
	, m_brightness(0.f, -1.f, 1.f, 3, PID_TEXTURE_BRIGHTNESS, *this)
	, m_contrast(0.f, -1.f, 1.f, 3, PID_TEXTURE_CONTRAST, *this)
	, m_gamma(1.f, 0.01f, 20.f, 2, PID_TEXTURE_GAMMA, *this)
	, m_isTransparent(false)
	, m_hasTransofmation(false)
	, m_hasCrop(false)
	, m_crop(0.f, 0.f, 1.f, 1.f)
{
	m_normapMap.setVisible(type == TYPE_BUMP_MAP);
	m_normapMap.setEnabled(type == TYPE_BUMP_MAP);
	if (type == TYPE_BUMP_MAP) {
		m_brightness.setVisible(false);
		m_brightness.setEnabled(false);
		m_contrast.setVisible(false);
		m_contrast.setEnabled(false);
		m_gamma.setVisible(false);
		m_gamma.setEnabled(false);
	}

	updateTransformation();

	connect(m_owner.core().appContext().imageManager(), SIGNAL(imageUpdated(const IImage *)), this, SLOT(onImageUpdated(const IImage *)), Qt::DirectConnection);
}

TextureImpl::~TextureImpl()
{
}

// ------------------------------------------------------------------------ //

void TextureImpl::_set(bool enabled, const QString & fileName, const RectF & crop,
					   eWrap wrapX, eWrap wrapY, eMapping mapping,
					   const vec2 & repeat, const vec2 & offset, float rotation,
					   bool invert, float brightness, float contrast, float gamma,
					   bool normalMap)
{
	m_enabled._set(enabled);
	m_fileName._set(fileName);
	m_cropLeft._set(crop.left);
	m_cropTop._set(crop.top);
	m_cropRight._set(crop.right);
	m_cropBottom._set(crop.bottom);
	m_wrapX._setIndex(wrapX);
	m_wrapY._setIndex(wrapY);
	m_mapping._setIndex(mapping);
	m_repeat._set(repeat);
	m_offset._set(offset);
	m_rotation._set(rotation);
	m_invert._set(invert);
	m_brightness._set(brightness);
	m_contrast._set(contrast);
	m_gamma._set(gamma);
	m_normapMap._set(normalMap);

	loadImage();
	updateTransformation();
}

void TextureImpl::onImageUpdated(const IImage *image)
{
	if (m_originImage.get() == image)
		updateImage();
}

void TextureImpl::loadImage(bool fileNameChanged)
{
	if (m_fileName.get().isEmpty()) {
		m_originImage.reset();
		m_image.reset();
		return;
	}

	try {
		m_originImage = m_owner.core().appContext().imageManager()->loadImage(m_fileName.get(), true, m_type == TYPE_COLOR ? eImageColorSpace::Linear : eImageColorSpace::Unknown);
	} catch (const std::exception &) {
		m_originImage.reset();
	}

	updateImage(fileNameChanged);
}

bool TextureImpl::allocateImage(int width, int height, eImageFormat format, eImageDataType dataType, eImageColorSpace colorSpace)
{
	if (!m_image.get() || m_image.get() == m_originImage.get() ||
		m_image->width() != width || m_image->height() != height ||
		m_image->format() != format || m_image->dataType() != dataType)
		m_image = core().appContext().imageManager()->createImage(width, height, format, dataType, colorSpace);

	return m_image.get() != nullptr;
}

void TextureImpl::updateImage(bool fileNameChanged)
{
	m_isTransparent = false;

	if (!m_originImage.get()) {
		m_image.reset();
		return;
	}

	const auto width = m_originImage->width();
	const auto height = m_originImage->height();
	const auto format = m_originImage->format();
	const auto dataType = m_originImage->dataType();
	const auto colorSpace = m_originImage->colorSpace();
	const bool isHDR = m_id == PID_SCENE_ENVIRONMENT || m_id == PID_SCENE_BACKGROUND || m_id == PID_GROUP_EMISSION ||
		m_id == PID_LAYER_ROUGHNESS || m_id == PID_LAYER_ANISOTROPY || m_id == PID_LAYER_ANISOTROPY_ANGLE || m_id == PID_LAYER_FILM_THICKNESS;

	m_isTransparent = checkIfImageHasTransparentPixels(m_originImage.get());

	if (m_type == TYPE_BUMP_MAP) {
		if (fileNameChanged)
			m_normapMap.set(checkIfImageIsNormalMap(m_originImage));

		if (m_normapMap.get()) {
			m_image = m_originImage;
		} else {
			const vec2 scale((float)width / 256.f, -(float)height / 256.f);

			if (!allocateImage(width, height, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Linear))
				return;

			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					vec3 normal;
					normal.x = (m_originImage->getPixel(x > 0 ? x - 1 : width - 1, y).r -
								m_originImage->getPixel(x + 1 < width ? x + 1 : 0, y).r) * scale.x;
					normal.y = (m_originImage->getPixel(x, y > 0 ? y - 1 : height - 1).r -
								m_originImage->getPixel(x, y + 1 < height ? y + 1 : 0).r) * scale.y;
					normal.z = 1.f / 128.f;
					normal = glm::normalize(normal);
					normal = normal * 0.5f + vec3(0.5f);
					m_image->setPixel(x, y, vec4(normal, m_originImage->getPixel(x, y).r));
				}
			}
		}
	} else if (m_brightness.get() != 0.f || m_contrast.get() != 0.f || m_gamma.get() != 1.f) {
		float cm = CONTRAST_COEF * (1.f + m_contrast.get()) / (CONTRAST_COEF - m_contrast.get());
		float ca = m_brightness.get() + 0.5f - 0.5f * cm;
		float gp = 1.f / m_gamma.get();

		if (!allocateImage(width, height, format, dataType, colorSpace))
			return;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				auto color = m_originImage->getPixel(x, y);
				vec3 rgb = vec3(color) * cm + ca;

				if (isHDR) {
					rgb = glm::max(rgb, 0.f);
				} else {
					rgb = glm::clamp(rgb, 0.f, 1.f);
				}

				if (gp != 1.f) {
					rgb = glm::pow(rgb, vec3(gp));
				}

				m_image->setPixel(x, y, vec4(rgb, color.a));
			}
		}
	} else if (dataType == eImageDataType::Float && !isHDR) {
		if (!allocateImage(width, height, format, dataType, colorSpace))
			return;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				auto color = m_originImage->getPixel(x, y);
				m_image->setPixel(x, y, ColorUtils::clamp(color));
			}
		}
	} else
		m_image = m_originImage;
}

void TextureImpl::updateTransformation()
{
	m_hasCrop = m_cropLeft.get() != 0.f || m_cropTop.get() != 0.f || m_cropRight.get() != 1.f || m_cropBottom.get() != 1.f;
	m_crop = RectF(m_cropLeft.get(), 1.f - m_cropBottom.get(), m_cropRight.get(), 1.f - m_cropTop.get());

	auto & repeat = m_repeat.get();
	auto & offset = m_offset.get();
	m_hasTransofmation = repeat.x != 1.f || repeat.y != 1.f || offset.x != 0.f || offset.y != 0.f || m_rotation.get() != 0.f;
	const float angle = glm::radians(m_rotation.get());
	const float sa = std::sin(angle), ca = std::cos(angle);
	m_tm[0].x = repeat.x * ca;
	m_tm[0].y = repeat.x * sa;
	m_tm[0].z = offset.x + 0.5f - 0.5f * (m_tm[0].x + m_tm[0].y);
	m_tm[1].x = repeat.y * -sa;
	m_tm[1].y = repeat.y * ca;
	m_tm[1].z = offset.y + 0.5f - 0.5f * (m_tm[1].x + m_tm[1].y);
}

// ------------------------------------------------------------------------ //

vec2 TextureImpl::getTC(const Ray & ray) const
{
	return ray.textureCoords(m_mapping.getIndex());
}

void TextureImpl::getTangentBinormal(vec3 & tangent, vec3 & binormal, const Ray & ray, float bumpDepth) const
{
	auto & uvSet = ray.geom->uvSet(m_mapping.getIndex());
	const auto dpdu = ray.dpdu();
	const auto dpdv = ray.dpdv();
	if (!uvSet.empty()) {
		auto & tc0 = uvSet[ray.vi[0]];
		auto dTC1 = uvSet[ray.vi[1]] - tc0;
		auto dTC2 = uvSet[ray.vi[2]] - tc0;
		auto f = dTC1.x * dTC2.y - dTC1.y * dTC2.x;
		bumpDepth *= f;
		tangent = (dpdu * dTC2.y - dpdv * dTC1.y);
		binormal = (dpdv * dTC1.x - dpdu * dTC2.x);
	} else {
		tangent = dpdu;
		binormal = dpdv;
	}

	auto tl2 = glm::length2(tangent);
	if (tl2 > 0.f)
		tangent *= bumpDepth / tl2;

	auto bl2 = glm::length2(binormal);
	if (bl2 > 0.f)
		binormal *= bumpDepth / bl2;

	tangent -= ray.normal * glm::dot(tangent, ray.normal);
	binormal -= ray.normal * glm::dot(binormal, ray.normal);
}

vec3 TextureImpl::getTangentDir(const Ray &ray) const
{
	return ray.tangentDir(m_mapping.getIndex());
}

// ------------------------------------------------------------------------ //

bool TextureImpl::isDefault() const
{
	return	m_fileName.get().isEmpty() &&
			m_enabled.get() &&
			!m_hasTransofmation && !m_hasCrop &&
			m_wrapX.getIndex() == WRAP_REPEAT && m_wrapY.getIndex() == WRAP_REPEAT &&
			m_mapping.getIndex() == MAPPING_UV0 &&
			!m_invert.get() && !m_normapMap.get() &&
			m_brightness.get() == 0.f && m_contrast.get() == 0.f && m_gamma.get() == 1.f;
}

#define TEXTURE						"texture"
#define TEXTURE_ENABLE				"enable"
#define TEXTURE_FILE_NAME			"file"
#define TEXTURE_CROP				"crop"
#define TEXTURE_OFFSET				"offset"
#define TEXTURE_REPEAT				"repeat"
#define TEXTURE_ROTATION			"rotation"
#define TEXTURE_NORMAL_MAP			"normal-map"
#define TEXTURE_INVERT				"invert"
#define TEXTURE_WRAP_X				"wrap-x"
#define TEXTURE_WRAP_Y				"wrap-y"
#define TEXTURE_MAPPING				"mapping"
#define TEXTURE_BRIGHTNESS			"brightness"
#define TEXTURE_CONTRAST			"contrast"
#define TEXTURE_GAMMA				"gamma"

bool TextureImpl::_load(const QDomElement & node, const ModelLoadingContext &ctx)
{
	if (node.isNull())
		return false;

	m_enabled.load(node, TEXTURE_ENABLE);
	m_fileName.load(node, TEXTURE_FILE_NAME, ctx);
	vec4 crop(0.f, 0.f, 1.f, 1.f);
	loadVec4Param(crop, node, TEXTURE_CROP);
	m_cropLeft._set(crop.x);
	m_cropTop._set(crop.y);
	m_cropRight._set(crop.z);
	m_cropBottom._set(crop.w);
	m_wrapX.load(node, TEXTURE_WRAP_X);
	m_wrapY.load(node, TEXTURE_WRAP_Y);
	m_mapping.load(node, TEXTURE_MAPPING);
	m_repeat.load(node, TEXTURE_REPEAT);
	m_offset.load(node, TEXTURE_OFFSET);
	m_rotation.load(node, TEXTURE_ROTATION);
	m_invert.load(node, TEXTURE_INVERT);
	if (m_type == TYPE_BUMP_MAP) {
		m_normapMap.load(node, TEXTURE_NORMAL_MAP);
	} else {
		m_brightness.load(node, TEXTURE_BRIGHTNESS);
		m_contrast.load(node, TEXTURE_CONTRAST);
		m_gamma.load(node, TEXTURE_GAMMA);
	}

	loadImage();
	updateTransformation();

	return true;
}

bool TextureImpl::_load(const QJsonObject & obj, const ModelLoadingContext &ctx)
{
	if (obj.isEmpty())
		return false;

	m_enabled.load(obj, TEXTURE_ENABLE);
	m_fileName.load(obj, TEXTURE_FILE_NAME, ctx);

	vec4 crop(0.f, 0.f, 1.f, 1.f);
	crop = getVec4(obj, TEXTURE_CROP, crop);
	m_cropLeft._set(crop.x);
	m_cropTop._set(crop.y);
	m_cropRight._set(crop.z);
	m_cropBottom._set(crop.w);

	m_wrapX.load(obj, TEXTURE_WRAP_X);
	m_wrapY.load(obj, TEXTURE_WRAP_Y);
	m_mapping.load(obj, TEXTURE_MAPPING);
	m_repeat.load(obj, TEXTURE_REPEAT);
	m_offset.load(obj, TEXTURE_OFFSET);
	m_rotation.load(obj, TEXTURE_ROTATION);
	m_invert.load(obj, TEXTURE_INVERT);

	if (m_type == TYPE_BUMP_MAP) {
		m_normapMap.load(obj, TEXTURE_NORMAL_MAP);
	} else {
		m_brightness.load(obj, TEXTURE_BRIGHTNESS);
		m_contrast.load(obj, TEXTURE_CONTRAST);
		m_gamma.load(obj, TEXTURE_GAMMA);
	}

	loadImage();
	updateTransformation();

	return true;
}

void TextureImpl::_save(QJsonObject & obj, const ModelSavingContext &ctx) const
{
	if (!m_enabled.get())
		m_enabled.save(obj, TEXTURE_ENABLE);

	m_fileName.save(obj, TEXTURE_FILE_NAME, ctx);

	if (m_hasCrop)
		obj[TEXTURE_CROP] = toJsonArray(vec4(m_cropLeft.get(), m_cropTop.get(), m_cropRight.get(), m_cropBottom.get()));

	m_wrapX.save(obj, TEXTURE_WRAP_X);
	m_wrapY.save(obj, TEXTURE_WRAP_Y);
	m_mapping.save(obj, TEXTURE_MAPPING);

	if (m_hasTransofmation) {
		m_repeat.save(obj, TEXTURE_REPEAT);
		m_offset.save(obj, TEXTURE_OFFSET);
		m_rotation.save(obj, TEXTURE_ROTATION);
	}

	if (m_invert.get())
		m_invert.save(obj, TEXTURE_INVERT);

	if (m_type == TYPE_BUMP_MAP) {
		m_normapMap.save(obj, TEXTURE_NORMAL_MAP);
	} else if (!m_brightness.isDefault() || !m_contrast.isDefault() || !m_gamma.isDefault()) {
		m_brightness.save(obj, TEXTURE_BRIGHTNESS);
		m_contrast.save(obj, TEXTURE_CONTRAST);
		m_gamma.save(obj, TEXTURE_GAMMA);
	}
}

// ------------------------------------------------------------------------ //

class TextureLoadCommand : public QUndoCommand
{
	struct State {
		bool	enabled;
		QString	fileName;
		RectF	crop;
		TextureImpl::eMapping mapping;
		vec2	repeat;
		vec2	offset;
		float	rotation;
		bool	invert;
		TextureImpl::eWrap	wrapX, wrapY;
		float	brightness;
		float	contrast;
		float	gamma;
		bool	normalMap;
	};

	TextureImpl & m_texture;
	State m_oldState;
	State m_newState;

	void getState(State & state)
	{
		state.enabled = m_texture.enabled().get();
		state.fileName = m_texture.fileName().get();
		state.crop.left = m_texture.cropLeft().get();
		state.crop.top = m_texture.cropTop().get();
		state.crop.right = m_texture.cropRight().get();
		state.crop.bottom = m_texture.cropBottom().get();
		state.wrapX = (TextureImpl::eWrap)m_texture.wrapX().getIndex();
		state.wrapY = (TextureImpl::eWrap)m_texture.wrapY().getIndex();
		state.mapping = (TextureImpl::eMapping)m_texture.mapping().getIndex();
		state.repeat = m_texture.repeat().get();
		state.offset = m_texture.offset().get();
		state.rotation = m_texture.rotation().get();
		state.invert = m_texture.invert().get();
		state.brightness = m_texture.brightness().get();
		state.contrast = m_texture.contrast().get();
		state.gamma = m_texture.gamma().get();
		state.normalMap = m_texture.normalMap().get();
	}

	void setState(const State & state)
	{
		auto & scene = m_texture.core().scene();
		scene.lock(SceneInternalModification_Texture);

		m_texture._set(state.enabled, state.fileName, state.crop, state.wrapX, state.wrapY,
							  state.mapping, state.repeat, state.offset, state.rotation,
							  state.invert, state.brightness, state.contrast, state.gamma, state.normalMap);
		m_texture.fireChanged(PID_TEXTURE);

		scene.unlock(SceneInternalModification_Texture);
	}

	void redo() override
	{
		setState(m_newState);
	}

	void undo() override
	{
		setState(m_oldState);
	}

public:
	TextureLoadCommand(TextureImpl & texture, const QByteArray & data, const ModelLoadingContext &ctx)
		: m_texture(texture)
	{
		getState(m_oldState);

		QJsonParseError parseError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
		if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull() && jsonDoc.isObject()) {
			m_texture._load(jsonDoc.object(), ctx);
		}

		getState(m_newState);
	}
};

bool TextureImpl::load(const QByteArray & data, const ModelLoadingContext &ctx)
{
	m_owner.pushCommand(new TextureLoadCommand(*this, data, ctx));
	return true;
}

QByteArray TextureImpl::save(const ModelSavingContext &ctx) const
{
	QJsonObject obj;
	_save(obj, ctx);
	return QJsonDocument(obj).toJson(ctx.targetFolder.isEmpty() ? QJsonDocument::Compact : QJsonDocument::Indented);
}

// ------------------------------------------------------------------------ //

int TextureImpl::getFlags() const
{
	int flags = 0;
	if (m_enabled.get())	flags |= (1 << 0);
	if (m_invert.get())		flags |= (1 << 1);
	if (m_normapMap.get())	flags |= (1 << 2);
	return flags;
}

QJsonObject TextureImpl::getState() const
{
	const int wrap[2] = { m_wrapX.getIndex(), m_wrapY.getIndex() };
	QJsonObject state {
		{ "flags", (int)getFlags() },
		{ "filename", m_fileName.get() },
		{ "wrap", toJsonArray(wrap, 2) },
		{ "mapping", m_mapping.getIndex() }
	};

	if (m_hasCrop) {
		const float crop[4] = { m_cropLeft.get(), m_cropTop.get(), m_cropRight.get(), m_cropBottom.get() };
		state["crop"] = toJsonArray(crop, 4);
	}

	if (m_hasTransofmation) {
		state["repeat"] = toJsonArray(m_repeat.get());
		state["offset"] = toJsonArray(m_offset.get());
		state["rotation"] = m_rotation.get();
	}

	if (!m_brightness.isDefault() || !m_contrast.isDefault() || !m_gamma.isDefault()) {
		state["brightness"] = m_brightness.get();
		state["contrast"] = m_contrast.get();
		state["gamma"] = m_gamma.get();
	}

	return state;
}

void TextureImpl::setState(const QJsonObject & state)
{
	if (state.isEmpty()) return;

	const auto flags = state.value("flags").toInt(getFlags());
	const auto filename = state.value("filename").toString(m_fileName.get());

	float crop[4] = { 0.f, 0.f, 1.f, 1.f };
	getArray(state, "crop", crop, 4);

	int wrap[2] = { m_wrapX.getIndex(), m_wrapY.getIndex() };
	getArray(state, "wrap", wrap, 2);
	const auto mapping = state.value("mapping").toInt(m_mapping.getIndex());

	const auto repeat = getVec2(state, "repeat", vec2(1.f));
	const auto offset = getVec2(state, "offset", vec2(0.f));
	const auto rotation = state.value("rotation").toDouble(0.f);

	const auto brightness = state.value("brightness").toDouble(0.f);
	const auto contrast = state.value("contrast").toDouble(0.f);
	const auto gamma = state.value("gamma").toDouble(1.f);

	if (flags == getFlags() && filename == m_fileName.get() &&
		m_cropLeft.get() == crop[0] && m_cropTop.get() == crop[1] && m_cropRight.get() == crop[2] && m_cropBottom.get() == crop[3] &&
		wrap[0] == m_wrapX.getIndex() && wrap[1] == m_wrapY.getIndex() && mapping == m_mapping.getIndex() &&
		repeat == m_repeat.get() && offset == m_offset.get() && rotation == m_rotation.get() &&
		brightness == m_brightness.get() && contrast == m_contrast.get() && gamma == m_gamma.get())
		return;

	_set(flags & (1 << 0), filename, RectF(crop[0], crop[1], crop[2], crop[3]),
		(ITexture::eWrap)wrap[0], (ITexture::eWrap)wrap[1], (eMapping)mapping,
		repeat, offset, rotation, flags & (1 << 1),
		brightness, contrast, gamma, flags & (1 << 2));

}

// ------------------------------------------------------------------------ //

bool TextureImpl::isEqual(const TextureImpl & other) const
{
	return (!m_enabled.get() && !other.m_enabled.get()) || (m_fileName.get().isEmpty() && other.m_fileName.get().isEmpty()) ||
		(m_enabled.get() == other.m_enabled.get() &&
		m_fileName.get() == other.m_fileName.get() &&
		m_crop.left == other.m_crop.left && m_crop.top == other.m_crop.top && m_crop.right == other.m_crop.right && m_crop.bottom == other.m_crop.bottom &&
		m_wrapX.getIndex() == other.m_wrapX.getIndex() &&
		m_wrapY.getIndex() == other.m_wrapY.getIndex() &&
		m_mapping.getIndex() == other.m_mapping.getIndex() &&
		m_repeat.get() == other.m_repeat.get() &&
		m_offset.get() == other.m_offset.get() &&
		m_rotation.get() == other.m_rotation.get() &&
		m_invert.get() == other.m_invert.get() &&
		m_brightness.get() == other.m_brightness.get() &&
		m_contrast.get() == other.m_contrast.get() &&
		m_gamma.get() == other.m_gamma.get() &&
		m_normapMap.get() == other.m_normapMap.get());
}

void TextureImpl::copyFrom(const TextureImpl &srcTexture)
{
	_set(srcTexture.m_enabled.get(), srcTexture.m_fileName.get(), srcTexture.m_crop,
		(eWrap)srcTexture.m_wrapX.getIndex(), (eWrap)srcTexture.m_wrapY.getIndex(), (eMapping)srcTexture.m_mapping.getIndex(),
		srcTexture.m_repeat.get(), srcTexture.m_offset.get(), srcTexture.m_rotation.get(), srcTexture.m_invert.get(),
		srcTexture.m_brightness.get(), srcTexture.m_contrast.get(), srcTexture.m_gamma.get(), srcTexture.m_normapMap.get());
}

void TextureImpl::getTransitionState(TransitionState & state) const
{
	state.image = m_image;
	state.isTransparent = m_isTransparent;
	state.hasTransofmation = m_hasTransofmation;
	state.hasCrop = m_hasCrop;
	state.crop = m_crop;
	state.tm[0] = m_tm[0];
	state.tm[1] = m_tm[1];
	state.invert = m_invert.get();
	state.wrap[0] = m_wrapX.getIndex();
	state.wrap[1] = m_wrapY.getIndex();
}

void TextureImpl::setTransitionState(const TransitionState & state)
{
	m_image = state.image;
	m_isTransparent = state.isTransparent;
	m_hasTransofmation = state.hasTransofmation;
	m_hasCrop = state.hasCrop;
	m_crop = state.crop;
	m_tm[0] = state.tm[0];
	m_tm[1] = state.tm[1];
	m_invert._set(state.invert);
	m_wrapX._setIndex(state.wrap[0]);
	m_wrapY._setIndex(state.wrap[1]);
}

// ------------------------------------------------------------------------ //

void TextureImpl::setEnabledMappingParams(bool enable)
{
	m_mapping.setVisible(false);
	m_offset.setVisible(false);
	m_repeat.setVisible(false);
	m_rotation.setVisible(false);
	m_wrapX.setVisible(enable);
	m_wrapY.setVisible(enable);
}

// ------------------------------------------------------------------------ //

QImage TextureImpl::getPreview() const
{
	if (!m_originImage)
		return QImage();

	const int IDEAL_WIDTH = 400, IDEAL_HEIGHT = 400;

	int width = IDEAL_WIDTH, height = IDEAL_HEIGHT;
	if (m_originImage->width() < IDEAL_WIDTH && m_originImage->height() < IDEAL_HEIGHT) {
		width = m_originImage->width();
		height = m_originImage->height();
	}

	if (width * m_originImage->height() > height * m_originImage->width())
		width = height * m_originImage->width() / m_originImage->height();
	else
		height = width * m_originImage->height() / m_originImage->width();
	QImage image(width, height, QImage::Format_ARGB32);

	float cm = 1.f, ca = 0.f, gp = 1.f;
	if (m_type != TYPE_BUMP_MAP) {
		cm = CONTRAST_COEF * (1.f + m_contrast.get()) / (CONTRAST_COEF - m_contrast.get());
		ca = m_brightness.get() + 0.5f - 0.5f * cm;
		gp = 1.f / m_gamma.get();
	}

	bool isNormalMap = m_type == TYPE_BUMP_MAP && m_normapMap.get();

	float du = 1.f / (float)width;
	float dv = 1.f / (float)height;
	for (int y = 0; y < height; y++) {
		float v = 1.f - y * dv;
		for (int x = 0; x < width; x++) {
			float u = x * du;
			auto color = m_originImage->getPixelUV(u, v, true, true);

			color.r = std::max(color.r * cm + ca, 0.f);
			color.g = std::max(color.g * cm + ca, 0.f);
			color.b = std::max(color.b * cm + ca, 0.f);

			if (gp != 1.f) {
				color.r = std::pow(color.r, gp);
				color.g = std::pow(color.g, gp);
				color.b = std::pow(color.b, gp);
			}

			if (m_invert) {
				color.r = 1.f - color.r;
				color.g = 1.f - color.g;
				if (!isNormalMap)
					color.b = 1.f - color.b;
			}

			if (m_type == TYPE_COLOR)
				color = ColorUtils::linearToSRGB(color);

			image.setPixel(x, y, qRgba(SF2B(color.r), SF2B(color.g), SF2B(color.b), SF2B(color.a)));
		}
	}

	return image;
}

// ------------------------------------------------------------------------ //

void TextureImpl::fireChanged(eParamId paramId)
{
	assert(!m_owner.core().scene().isValid() && "Scene must be locked!");

	switch (paramId) {
		case PID_TEXTURE_FILENAME:
			loadImage(true);
			break;

		case PID_TEXTURE_NORMAL_MAP:
		case PID_TEXTURE_BRIGHTNESS:
		case PID_TEXTURE_CONTRAST:
		case PID_TEXTURE_GAMMA:
			updateImage();
			break;

		case PID_TEXTURE_CROP:
		case PID_TEXTURE_REPEAT:
		case PID_TEXTURE_OFFSET:
		case PID_TEXTURE_ROTATION:
			updateTransformation();
			break;
	}

	m_owner.fireChanged(m_id);
}

void TextureImpl::updateFileNames(const QMap<QString, QString>& map, ITexture* texture)
{
	if (!texture) return;

	const auto fileName = TextureImpl::safeGetFileName(texture);
	if (map.contains(fileName))
		texture->fileName().set(map[fileName]);
}
