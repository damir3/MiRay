#include "ColorUtils.h"
#include <QTextStream>

namespace ColorUtils {

QString toString(const glm::vec4 & c)
{
	return (c.a == 1.f) ? QString().asprintf("%g %g %g", c.r, c.g, c.b) : QString().asprintf("%g %g %g %g", c.r, c.g, c.b, c.a);
}

glm::vec4 fromString(QString str, const glm::vec4 & def)
{
	str = str.trimmed();
	if (str.isEmpty())
		return def;

	if (str[0] == '#' || (str[0] >= 'a' && str[0] <= 'z') || (str[0] >= 'A' && str[0] <= 'Z')) {
		QColor clr(str);
		if (clr.isValid())
			return glm::vec4(clr.redF(), clr.greenF(), clr.blueF(), clr.alphaF());
		return def;
	}

	glm::vec4 out;
	QTextStream stm(&str);
	stm >> out.r >> out.g >> out.b;
	if (stm.status() != QTextStream::Ok)
		return def;

	stm >> out.a;
	if (stm.status() != QTextStream::Ok)
		out.a = 1.f;

	return out;
}

} // namespace ColorUtils
