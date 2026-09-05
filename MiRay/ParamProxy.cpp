#include "ParamProxy.h"
#include "MainWindow.h"

#include "../Shared/Interfaces/Parameters.h"
#include "../Shared/Utils/FileUtils.h"

#include <QUrl>

static const QString SETTINGS_LAST_TEXTURE_FILE = "lastTextureFile";

ParamProxy::~ParamProxy()
{
	if (m_param)
		disconnect(m_param, SIGNAL(changed()), this, SLOT(onChanged()));
}

bool ParamProxy::getEnabled() const
{
	return m_param ? m_param->isEnabled() : false;
}

bool ParamProxy::getVisible() const
{
	return m_param ? m_param->isVisible() : false;
}

void ParamProxy::setParam(IParameter * param)
{
	if (m_param)
		disconnect(m_param, SIGNAL(changed()), this, SLOT(onChanged()));

	m_param = param;
	updateValue();
	emit changed();

	if (m_param)
		connect(m_param, SIGNAL(changed()), this, SLOT(onChanged()));
}

void ParamProxy::onChanged()
{
	updateValue();
	emit changed();
}

// ------------------------------------------------------------------------ //

void StringParamProxy::updateValue()
{
	auto param = static_cast<IStringParameter *>(m_param);
	m_value = param ? param->get() : QString();
}

void StringParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IStringParameter *>(m_param))
		param->set(value.toString());
}

// ------------------------------------------------------------------------ //

void FileNameParamProxy::updateValue()
{
	auto param = static_cast<IFileNameParameter *>(m_param);
	m_value = param ? param->get() : QString();
}

void FileNameParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IFileNameParameter *>(m_param)) {
		param->set(urlToLocalFile(value));
	}
}

void FileNameParamProxy::openFileDialog()
{
	if (auto param = static_cast<IFileNameParameter *>(m_param)) {
		QString initialPath = param->get();
		if (initialPath.isEmpty()) {
			initialPath = m_settings.value(m_lastFileSettingName).toString();
		}
		if (initialPath.isEmpty()) {
			initialPath = standardDesktopLocation();
		}
		const auto fileName = getOpenFileName(MainWindow::instance()->centralWidget(), "Please choose a file", "", initialPath, param->extensions());
		if (!fileName.isEmpty()) {
			param->set(fileName);
			m_settings.setValue(m_lastFileSettingName, fileName);
		}
	}
}

// ------------------------------------------------------------------------ //

void BooleanParamProxy::updateValue()
{
	auto param = static_cast<IBooleanParameter *>(m_param);
	m_value = param ? param->get() : false;
}

void BooleanParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IBooleanParameter *>(m_param))
		param->set(value.toBool());
}

// ------------------------------------------------------------------------ //

void EnumParamProxy::setParam(IParameter * param)
{
	ParamProxy::setParam(param);
	emit modelChanged();
}

QStringList EnumParamProxy::getModel() const
{
	QStringList list;
	if (auto param = static_cast<IEnumParameter *>(m_param)) {
		for (int i = 0; i < param->count(); i++)
			list.append(param->getTitle(i));
	}
	return list;
}

void EnumParamProxy::updateValue()
{
	auto param = static_cast<IEnumParameter *>(m_param);
	m_value = param ? param->getIndex() : 0;
}

void EnumParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IEnumParameter *>(m_param))
		param->setIndex(value.toInt());
}

// ------------------------------------------------------------------------ //

void ColorParamProxy::updateValue()
{
	auto param = static_cast<IColorParameter *>(m_param);

	const auto color = param ? param->get() : vec4(1.f, 1.f, 1.f, 1.f);
	m_value = QColor::fromRgbF(color.r, color.g, color.b);

	const auto texture = param ? param->texture() : nullptr;
	if (texture && !m_texture.data())
		m_texture.reset(new TextureParamProxy(m_textureTitle));

	if (m_texture.data())
		m_texture->setTexture(texture);
}

void ColorParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IColorParameter *>(m_param)) {
		const auto qcolor = value.value<QColor>();
		param->set(vec3(qcolor.redF(), qcolor.greenF(), qcolor.blueF()));
	}
}

QVariant ColorParamProxy::getTexture() const
{
	return m_texture ? m_texture->data() : QVariant();
}

// ------------------------------------------------------------------------ //

void IntegerParamProxy::updateValue()
{
	auto param = static_cast<IIntegerParameter *>(m_param);
	m_value = param ? param->get() : 0;
}

void IntegerParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IIntegerParameter *>(m_param))
		param->set(value.toInt());
}

// ------------------------------------------------------------------------ //

void ScalarParamProxy::updateValue()
{
	auto param = getParam();
	m_value = param ? QString::number(param->get(), 'f', param->precision()) : "0";

	if (param && param->texture() && !m_texture.data())
		m_texture.reset(new TextureParamProxy(m_textureTitle));

	if (m_texture.data())
		m_texture->setTexture(param ? param->texture() : nullptr);

	emit minMaxChanged();
}

void ScalarParamProxy::setValue(QVariant value)
{
	if (auto param = getParam())
		param->set(value.toFloat());
}

QVariant ScalarParamProxy::getTexture() const
{
	return m_texture ? m_texture->data() : QVariant();
}

float ScalarParamProxy::getMin() const
{
	return m_param ? getParam()->min() : 0.f;
}

float ScalarParamProxy::getMax() const
{
	return m_param ? getParam()->max() : 1.f;
}

int ScalarParamProxy::getPrecision() const
{
	return m_param ? getParam()->precision() : 1;
}

// ------------------------------------------------------------------------ //

void Vec2ParamProxy::updateValue()
{
	auto param = static_cast<IVec2Parameter *>(m_param);
	const auto v = param ? param->get() : vec2(0.f);
	const auto precision = param ? param->precision() : 0;
	auto map = m_value.toMap();
	map["x"] = QString::number(v.x, 'f', precision);
	map["y"] = QString::number(v.y, 'f', precision);
	m_value = map;
}

void Vec2ParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IVec2Parameter *>(m_param)) {
		auto map = value.toMap();
		param->set(vec2(map["x"].toFloat(), map["y"].toFloat()));
	}
}

// ------------------------------------------------------------------------ //

void Vec3ParamProxy::updateValue()
{
	auto param = static_cast<IVec3Parameter *>(m_param);
	const auto v = param ? param->get() : vec3(0.f);
	const auto precision = param ? param->precision() : 0;
	auto map = m_value.toMap();
	map["x"] = QString::number(v.x, 'f', precision);
	map["y"] = QString::number(v.y, 'f', precision);
	map["z"] = QString::number(v.z, 'f', precision);
	m_value = map;
}

void Vec3ParamProxy::setValue(QVariant value)
{
	if (auto param = static_cast<IVec3Parameter *>(m_param)) {
		auto map = value.toMap();
		param->set(vec3(map["x"].toFloat(), map["y"].toFloat(), map["z"].toFloat()));
	}
}

// ------------------------------------------------------------------------ //

TextureParamProxy::TextureParamProxy(const QString & title)
	: m_title(title)
	, m_texture(nullptr)
	, m_enabled("Enabled")
	, m_fileName("File", MainWindow::instance()->settings(), SETTINGS_LAST_TEXTURE_FILE)
	, m_normalMap("Normal Map")
	, m_invert("Invert")
	, m_mapping("Mapping")
	, m_repeat("Repeat")
	, m_offset("Offset")
	, m_rotation("Rotation")
	, m_cropLeft("Left")
	, m_cropTop("Top")
	, m_cropRight("Right")
	, m_cropBottom("Bottom")
	, m_wrapX("Wrap X")
	, m_wrapY("Wrap Y")
	, m_brightness("Brightness")
	, m_contrast("Contrast")
	, m_gamma("Gamma")
{
	m_data["title"] = m_title;
	m_data["enabled"] = QVariant::fromValue(static_cast<QObject *>(&m_enabled));
	m_data["fileName"] = QVariant::fromValue(static_cast<QObject *>(&m_fileName));
	m_data["normalMap"] = QVariant::fromValue(static_cast<QObject *>(&m_normalMap));
	m_data["invert"] = QVariant::fromValue(static_cast<QObject *>(&m_invert));
	m_data["mapping"] = QVariant::fromValue(static_cast<QObject *>(&m_mapping));
	m_data["repeat"] = QVariant::fromValue(static_cast<QObject *>(&m_repeat));
	m_data["offset"] = QVariant::fromValue(static_cast<QObject *>(&m_offset));
	m_data["rotation"] = QVariant::fromValue(static_cast<QObject *>(&m_rotation));
	m_data["cropLeft"] = QVariant::fromValue(static_cast<QObject *>(&m_cropLeft));
	m_data["cropTop"] = QVariant::fromValue(static_cast<QObject *>(&m_cropTop));
	m_data["cropRight"] = QVariant::fromValue(static_cast<QObject *>(&m_cropRight));
	m_data["cropBottom"] = QVariant::fromValue(static_cast<QObject *>(&m_cropBottom));
	m_data["wrapX"] = QVariant::fromValue(static_cast<QObject *>(&m_wrapX));
	m_data["wrapY"] = QVariant::fromValue(static_cast<QObject *>(&m_wrapY));
	m_data["brightness"] = QVariant::fromValue(static_cast<QObject *>(&m_brightness));
	m_data["contrast"] = QVariant::fromValue(static_cast<QObject *>(&m_contrast));
	m_data["gamma"] = QVariant::fromValue(static_cast<QObject *>(&m_gamma));
	m_data["texture"] = QVariant::fromValue(static_cast<QObject *>(this));
}

void TextureParamProxy::setTexture(ITexture * texture)
{
	m_texture = texture;
	m_enabled.setParam(texture ? &texture->enabled() : nullptr);
	m_fileName.setParam(texture ? &texture->fileName() : nullptr);
	m_normalMap.setParam(texture ? &texture->normalMap() : nullptr);
	m_invert.setParam(texture ? &texture->invert() : nullptr);
	m_mapping.setParam(texture ? &texture->mapping() : nullptr);
	m_repeat.setParam(texture ? &texture->repeat() : nullptr);
	m_offset.setParam(texture ? &texture->offset() : nullptr);
	m_rotation.setParam(texture ? &texture->rotation() : nullptr);
	m_cropLeft.setParam(texture ? &texture->cropLeft() : nullptr);
	m_cropTop.setParam(texture ? &texture->cropTop() : nullptr);
	m_cropRight.setParam(texture ? &texture->cropRight() : nullptr);
	m_cropBottom.setParam(texture ? &texture->cropBottom() : nullptr);
	m_wrapX.setParam(texture ? &texture->wrapX() : nullptr);
	m_wrapY.setParam(texture ? &texture->wrapY() : nullptr);
	m_brightness.setParam(texture ? &texture->brightness() : nullptr);
	m_contrast.setParam(texture ? &texture->contrast() : nullptr);
	m_gamma.setParam(texture ? &texture->gamma() : nullptr);

	emit previewChanged();
}

QString TextureParamProxy::getPreview()
{
	QString preview;
	if (m_texture) {
		auto image = m_texture->getPreview();
		if (!image.isNull()) {
			QByteArray byteArray;
			QBuffer buffer(&byteArray);
			buffer.open(QIODevice::WriteOnly);
			image.save(&buffer, "PNG");
			preview = "data:image/png;base64," + byteArray.toBase64();
		}
	}
	return preview;
}
