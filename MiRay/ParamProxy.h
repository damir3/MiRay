#pragma once

#include <QQuickWidget>
#include <QDialog>

class ParamProxy : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString title MEMBER m_title CONSTANT)
	Q_PROPERTY(bool enabled READ getEnabled NOTIFY changed)
	Q_PROPERTY(bool visible READ getVisible NOTIFY changed)
	Q_PROPERTY(QVariant value READ getValue WRITE setValue NOTIFY changed)

protected:
	const QString m_title;

	IParameter * m_param;
	QVariant m_value;

public:
	explicit ParamProxy(const QString & title) : m_title(title), m_param(nullptr) {}
	~ParamProxy() override;

	bool getEnabled() const;
	bool getVisible() const;

	virtual void setParam(IParameter * param);

	virtual void updateValue() = 0;

	QVariant getValue() const { return m_value; }
	virtual void setValue(QVariant value) = 0;

private slots:
	void onChanged();

signals:
	void changed();
};

class StringParamProxy : public ParamProxy
{
	Q_OBJECT

public:
	explicit StringParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }

	void updateValue() override;
	void setValue(QVariant value) override;
};

class QSettings;

class FileNameParamProxy : public ParamProxy
{
	Q_OBJECT

	QSettings & m_settings;
	QString m_lastFileSettingName;

public:
	explicit FileNameParamProxy(const QString & title, QSettings & settings, const QString & lastFileSettingName)
		: ParamProxy(title)
		, m_settings(settings)
		, m_lastFileSettingName(lastFileSettingName)
	{
		updateValue();
	}

	void updateValue() override;
	void setValue(QVariant value) override;

	Q_INVOKABLE void openFileDialog();
};

class BooleanParamProxy : public ParamProxy
{
	Q_OBJECT

public:
	explicit BooleanParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }

	void updateValue() override;
	void setValue(QVariant value) override;
};

class EnumParamProxy : public ParamProxy
{
	Q_OBJECT

	Q_PROPERTY(QStringList model READ getModel NOTIFY modelChanged)

	QStringList getModel() const;

public:
	explicit EnumParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }

	void setParam(IParameter * param) override;

	void updateValue() override;
	void setValue(QVariant value) override;

signals:
	void modelChanged();
};

class TextureParamProxy;

class ColorParamProxy : public ParamProxy
{
	Q_OBJECT

	Q_PROPERTY(QVariant texture READ getTexture CONSTANT)

	const QString m_textureTitle;
	QScopedPointer<TextureParamProxy> m_texture;

public:
	explicit ColorParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }
	explicit ColorParamProxy(const QString & title, const QString & textureTitle) : ParamProxy(title), m_textureTitle(textureTitle) { updateValue(); }

	void updateValue() override;
	void setValue(QVariant value) override;

	QVariant getTexture() const;
};

class IntegerParamProxy : public ParamProxy
{
	Q_OBJECT

public:
	explicit IntegerParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }

	void updateValue() override;
	void setValue(QVariant value) override;
};

class ScalarParamProxy : public ParamProxy
{
	Q_OBJECT

	Q_PROPERTY(QVariant texture READ getTexture CONSTANT)
	Q_PROPERTY(float min READ getMin NOTIFY minMaxChanged)
	Q_PROPERTY(float max READ getMax NOTIFY minMaxChanged)
	Q_PROPERTY(int precision READ getPrecision NOTIFY minMaxChanged)

	const QString m_textureTitle;
	QScopedPointer<TextureParamProxy> m_texture;
	IScalarParameter * getParam() const { return static_cast<IScalarParameter *>(m_param); }

public:
	explicit ScalarParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }
	explicit ScalarParamProxy(const QString & title, const QString & textureTitle) : ParamProxy(title), m_textureTitle(textureTitle) { updateValue(); }

	void updateValue() override;
	void setValue(QVariant value) override;

	QVariant getTexture() const;
	float getMin() const;
	float getMax() const;
	int getPrecision() const;

signals:
	void minMaxChanged();
};

class Vec2ParamProxy : public ParamProxy
{
	Q_OBJECT

public:
	explicit Vec2ParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }

	void updateValue() override;
	void setValue(QVariant value) override;
};

class Vec3ParamProxy : public ParamProxy
{
	Q_OBJECT

public:
	explicit Vec3ParamProxy(const QString & title) : ParamProxy(title) { updateValue(); }

	void updateValue() override;
	void setValue(QVariant value) override;
};

class TextureParamProxy : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString preview READ getPreview NOTIFY previewChanged)

	const QString m_title;
	ITexture * m_texture;

	BooleanParamProxy m_enabled;

	FileNameParamProxy m_fileName;

	BooleanParamProxy m_normalMap;

	BooleanParamProxy m_invert;

	EnumParamProxy m_mapping;

	Vec2ParamProxy m_repeat;
	Vec2ParamProxy m_offset;
	ScalarParamProxy m_rotation;

	ScalarParamProxy m_cropLeft;
	ScalarParamProxy m_cropTop;
	ScalarParamProxy m_cropRight;
	ScalarParamProxy m_cropBottom;

	EnumParamProxy m_wrapX;
	EnumParamProxy m_wrapY;

	ScalarParamProxy m_brightness;
	ScalarParamProxy m_contrast;
	ScalarParamProxy m_gamma;

	QString m_preview;

	QVariantMap m_data;

public:
	explicit TextureParamProxy(const QString & title);

	void setTexture(ITexture * texture);

	QVariant data() { return m_data; }
	QString getPreview();

signals:
	void previewChanged();
};
