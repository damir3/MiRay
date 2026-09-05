#pragma once

#include <QVariantMap>
#include <QStringList>

class ParametersParser
{
	QVariantMap m_params;
	QStringList m_files;

public:
	ParametersParser(QStringList);
	QVariantMap parsedArgs() const;
	QStringList parsedFilenames() const;

	static bool isJobManager(int argc, char* argv[]);
};
