#include "FileUtils.h"

#include <QFileDialog>
#include <QDebug>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

QByteArray readFile(const QString & fileName)
{
	QFile f(fileName);
	if (!f.open(QIODevice::ReadOnly))
		throw std::runtime_error(QString("Can't open file %1").arg(fileName).toStdString());
	return f.readAll();
}

void writeFile(const QString & fileName, const QByteArray & data)
{
	QFile f(fileName);
	if (!f.open(QIODevice::WriteOnly))
		throw std::runtime_error(QString("Can't open file %1").arg(fileName).toStdString());

	if (data.length() != f.write(data))
		throw std::runtime_error(QString("Can't write the data to file %1").arg(fileName).toStdString());

	f.close();
}

// ------------------------------------------------------------------------ //

QString sanitizeFilename(const QString &fileName, const QString& replaceWith)
{
	static const QRegularExpression regex(QStringLiteral(R"([\\/:*?"<>|])"));
	QString newFileName = fileName;
	return newFileName.replace(regex, replaceWith);
}

QString nativePath(const QString & path)
{
#ifdef __APPLE__
	return QString(path).replace('\\', '/');
#else
	return QString(path).replace('/', '\\');
#endif
}

QString relativePath(const QString &filePath, const QString &dir)
{
	if (filePath.isEmpty() || dir.isEmpty() || filePath.startsWith(':') || filePath.contains("://"))
		return filePath;

	QDir d(dir);
	QString rel = d.relativeFilePath(filePath);

	if (rel.startsWith("../") || rel == "..")
		return filePath;

	return rel;
}

QString absolutePath(const QString &filePath, const QString &dir)
{
	if (filePath.isEmpty())
		return QString();

	if (filePath.contains("://"))
		return filePath;

	if (dir.isEmpty() || filePath.startsWith(':'))
		return nativePath(filePath);

	if (QDir::isAbsolutePath(filePath) || (filePath.length() >= 2 && filePath[1] == ':' && filePath[0].isLetter()))
		return nativePath(filePath);

	return nativePath(QDir(dir).absoluteFilePath(filePath));
}

// ------------------------------------------------------------------------ //

QString urlToLocalFile(const QString & url)
{
	if (url.startsWith("file://")) {
		const QUrl qurl(url);
		if (qurl.isLocalFile())
			return qurl.toLocalFile();
	}
	return url;
}

QString urlToLocalFile(const QVariant & value)
{
	if (value.userType() == QMetaType::QUrl) {
		const auto url = value.toUrl();
		return url.isLocalFile() ? url.toLocalFile() : url.toString();
	}
	return urlToLocalFile(value.toString());
}

// ------------------------------------------------------------------------ //

QString findExistingFile(const QString &filePath, const QString &dir)
{
	if (filePath.isEmpty())
		return QString();

	if (filePath.startsWith(':') || filePath.contains("://"))
		return filePath;

	// check direct path (either absolute or relative to dir)
	const QString directPath = absolutePath(filePath, dir);
	QFileInfo directInfo(directPath);
	if (directInfo.exists() && directInfo.isFile())
		return nativePath(directInfo.canonicalFilePath());

	// check by filename directly inside dir (fallback if file was relocated)
	if (!dir.isEmpty()) {
		const QString fileName = QFileInfo(QDir::fromNativeSeparators(filePath)).fileName();
		if (!fileName.isEmpty() && fileName != filePath) {
			const QString altPath = absolutePath(fileName, dir);
			QFileInfo altInfo(altPath);
			if (altInfo.exists() && altInfo.isFile())
				return nativePath(altInfo.canonicalFilePath());
		}
	}

	return QString();
}

QString resolveFilePath(const QString &filePath, const QString &dir)
{
	if (filePath.isEmpty())
		return QString();

	const QString existing = findExistingFile(filePath, dir);
	if (!existing.isEmpty())
		return existing;

	return absolutePath(filePath, dir);
}

// ------------------------------------------------------------------------ //

QString standardDesktopLocation()
{
	return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

QString standardAppDataLocation()
{
	return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString standardTempLocation()
{
	return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

// ------------------------------------------------------------------------ //

static int getFilterIndex(const FileFormatsList &filters, const QString &extension)
{
	for (size_t i = 0; i < filters.size(); i++) {
		for (const auto &ext : filters[i].extensions) {
			if (ext.compare(extension, Qt::CaseInsensitive) == 0)
				return static_cast<int>(i);
		}
	}
	return -1;
}

QStringList buildFilterList(const FileFormatsList &filters)
{
	QStringList filterList;
	for (const auto &filter : filters) {
		QString exts;
		for (const auto &ext : filter.extensions) {
			if (!exts.isEmpty()) exts += " ";
			exts += QString("*.%1").arg(ext);
		}
		filterList << QString("%1 (%2)").arg(filter.description, exts);
	}
	return filterList;
}

QString buildFilterString(const FileFormatsList &filters)
{
	if (filters.empty()) return QString();

	return buildFilterList(filters).join(";;");
}

QString collectAllExtensions(const FileFormatsList &filters)
{
	QString allExtensions;
	for (const auto &filter : filters) {
		for (const auto &ext : filter.extensions) {
			if (!allExtensions.isEmpty()) allExtensions += " ";
			allExtensions += QString("*.%1").arg(ext);
		}
	}
	return allExtensions;
}

QString getOpenFileName(QWidget *parent, const QString &title, const QString &defaultExt, const QString &directory, const FileFormatsList &filters)
{
	if (filters.empty()) return QString();

	const auto individualFilters = buildFilterList(filters);
	const auto allSupported = "All Supported Formats (" + collectAllExtensions(filters) + ")";
	const auto allFilters = allSupported + ";;" + individualFilters.join(";;");

	// select the default filter matching defaultext (case-insensitive)
	const auto index = getFilterIndex(filters, defaultExt);
	auto selectedFilter = index >= 0 ? individualFilters[index] : QString();

	const auto str = QFileDialog::getOpenFileName(parent, title, directory, allFilters, selectedFilter.isEmpty() ? nullptr : &selectedFilter);
	if (str.isEmpty())
		return QString(); // user cancelled

	// validate that the chosen file's extension is among the allowed filters
	QString suffix = QFileInfo(str).suffix();
	if (getFilterIndex(filters, suffix) < 0)
		return QString();

	return QDir::toNativeSeparators(QFileInfo(str).absoluteFilePath());
}

QString getSaveFileName(QWidget *parent, const QString &title, const QString &defaultExt, const QString &fileName, const FileFormatsList &filters)
{
	if (filters.empty()) return QString();

	QStringList filterList = buildFilterList(filters);
	QString allFilter = filterList.join(";;");

	// select the default filter by matching defaultext against each filter's extensions
	const auto index = getFilterIndex(filters, defaultExt);
	auto selectedFilter = index >= 0 ? filterList[index] : QString();

	QString str = QFileDialog::getSaveFileName(parent, title, fileName, allFilter, &selectedFilter);

	if (str.isEmpty()) return QString(); // user cancelled

	// validate that the chosen extension matches one of the filters;
	// if not, auto-append the extension from the selected filter (matches save dialog behaviour)
	QString suffix = QFileInfo(str).suffix();
	if (getFilterIndex(filters, suffix) < 0)
	{
		QString ext = QString(".%1").arg(filters[index].extensions.first());
		if (!str.endsWith(ext))
			str += ext;
	}

	return QDir::toNativeSeparators(str);
}
