#include "SimpleParams.h"

SimpleIntegerParameter::SimpleIntegerParameter(int value, int min, int max)
	: m_value(value)
	, m_min(min)
	, m_max(max)
	, m_visible(true)
	, m_enabled(true)
{
}

void SimpleIntegerParameter::set(int value)
{
	value = std::clamp(value, m_min, m_max);
	if (m_value != value) {
		m_value = value;
		emit changed();
	}
}

void SimpleIntegerParameter::_set(int value)
{
	m_value = std::clamp(value, m_min, m_max);
}

SimpleScalarParameter::SimpleScalarParameter(float value, float min, float max, int precision)
	: m_value(value)
	, m_min(min)
	, m_max(max)
	, m_precision(precision)
	, m_visible(true)
	, m_enabled(true)
{
}

void SimpleScalarParameter::set(float value)
{
	m_value = std::clamp(value, m_min, m_max);
	emit changed();
}

void SimpleScalarParameter::_set(float value)
{
	m_value = std::clamp(value, m_min, m_max);
}

SimpleEnumParameter::SimpleEnumParameter(int index, const QStringList & items)
	: m_items(items)
	, m_count(items.count())
	, m_index(index)
	, m_enabled(true)
	, m_visible(true)
{
}
