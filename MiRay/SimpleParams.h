#pragma once

class SimpleBooleanParameter final : public IBooleanParameter
{
	Q_OBJECT

protected:
	bool		m_value;
	bool		m_visible;
	bool		m_enabled;

public:
	explicit SimpleBooleanParameter(bool value) : m_value(value), m_visible(true), m_enabled(true) {}

	bool get() const override { return m_value; }
	void set(bool b) override { m_value = b; emit changed(); }
	void _set(bool b) override { m_value = b; }

	bool isEnabled() const override { return m_enabled; }
	void setEnabled(bool b) { m_enabled = b; }

	bool isVisible() const override { return m_visible; }
	void setVisible(bool b) { m_visible = b; }
};

class SimpleIntegerParameter final : public IIntegerParameter
{
	Q_OBJECT

protected:
	int		m_value;
	const int		m_min;
	const int		m_max;
	bool	m_enabled;
	bool	m_visible;

public:
	explicit SimpleIntegerParameter(int value, int min, int max);

	int min() const override { return m_min; }
	int max() const override { return m_max; }

	int get() const override { return m_value; }
	void set(int value) override;
	void _set(int value) override;

	bool isEnabled() const override { return m_enabled; }
	void setEnabled(bool b) { m_enabled = b; }

	bool isVisible() const override { return m_visible; }
	void setVisible(bool b) { m_visible = b; }
};

class SimpleScalarParameter final : public IScalarParameter
{
	Q_OBJECT

protected:
	float	m_value;
	const float	m_min;
	const float	m_max;
	const int m_precision;
	bool	m_enabled;
	bool	m_visible;

public:
	explicit SimpleScalarParameter(float value, float min, float max, int precision);

	float min() const override { return m_min; }
	float max() const override { return m_max; }
	virtual int precision() const override { return m_precision; }

	float get() const override { return m_value; }
	void set(float value) override;
	void _set(float value) override;

	ITexture * texture() override { return nullptr; }
	const ITexture * texture() const override { return nullptr; }

	bool isEnabled() const override { return m_enabled; }
	void setEnabled(bool b) { m_enabled = b; }

	bool isVisible() const override { return m_visible; }
	void setVisible(bool b) { m_visible = b; }
};

class SimpleEnumParameter final : public IEnumParameter
{
	Q_OBJECT

protected:
	const QStringList m_items;
	const int	m_count;
	int			m_index;
	bool		m_enabled;
	bool		m_visible;

public:
	explicit SimpleEnumParameter(int index, const QStringList & items);

	int count() const override { return m_count; }
	QString getTitle(int i) const override { return m_items[i]; }

	int getIndex() const override { return m_index; }
	void setIndex(int index) override { m_index = index; emit changed(); }
	void _setIndex(int index) override { m_index = index; }

	bool isEnabled() const override { return m_enabled; }
	void setEnabled(bool b) { m_enabled = b; }

	bool isVisible() const override { return m_visible; }
	void setVisible(bool b) { m_visible = b; }
};

class SimpleStringParameter final : public IStringParameter
{
	Q_OBJECT

protected:
	QString m_value;
	bool    m_enabled;
	bool    m_visible;

public:
	explicit SimpleStringParameter(const QString &value) : m_value(value), m_enabled(true), m_visible(true) {}

	const QString & get() const override { return m_value; }
	void set(const QString &b) override { m_value = b; emit changed(); }
	void _set(const QString &b) override { m_value = b; }

	bool isEnabled() const override { return m_enabled; }
	void setEnabled(bool b) { m_enabled = b; }

	bool isVisible() const override { return m_visible; }
	void setVisible(bool b) { m_visible = b; }
};

