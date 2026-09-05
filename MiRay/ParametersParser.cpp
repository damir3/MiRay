#include "ParametersParser.h"
#include <QString>

ParametersParser::ParametersParser(QStringList strList)
{
	if (strList.size() <= 1) return;
	strList.pop_front(); // remove application name

	bool fileNamesOnly = false;

	foreach (auto str, strList)
	{
		if (str == "--" && !fileNamesOnly) {
			fileNamesOnly = true;
			continue;
		}

		if (!str.startsWith("--") || fileNamesOnly) {
			m_files << str;
			fileNamesOnly = true; // there can't be any options after a filename
			continue;
		}

		str.remove(0, 2); // remove --

		auto l = str.split("=");

		QString var = l[0];
		l.pop_front();

		QString val;
		if (l.size() > 0)
			val = l.join("=");

		if (val.startsWith('\"') && val.endsWith('\"') && val.length() > 1) // remove quotation
			val = val.mid(1, val.length() - 2);

		m_params[var] = val;
	}
}

QVariantMap ParametersParser::parsedArgs() const
{
	return m_params;
}

QStringList ParametersParser::parsedFilenames() const
{
	return m_files;
}

bool ParametersParser::isJobManager(int argc, char* argv[])
{
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			const auto  str = QString::fromLocal8Bit(argv[i]);
			if (str == "--jobs") {
				return true;
			}
		}
	}

	return false;
}
