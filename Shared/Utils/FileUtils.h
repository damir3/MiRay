#pragma once

#include <QVariant>

class QWidget;

struct FileFormat {
	QString description;
	QStringList extensions;
};

using FileFormatsList = std::vector<FileFormat>;

QByteArray SHAREDLIB_EXPORT readFile(const QString & fileName); // throws std::runtime_error
void SHAREDLIB_EXPORT writeFile(const QString & fileName, const QByteArray & data); // throws std::runtime_error

QString SHAREDLIB_EXPORT sanitizeFilename(const QString &fileName, const QString& replaceWith = "_");
QString SHAREDLIB_EXPORT nativePath(const QString & path);
QString SHAREDLIB_EXPORT relativePath(const QString &filePath, const QString &dir);
QString SHAREDLIB_EXPORT absolutePath(const QString &filePath, const QString &dir);

QString SHAREDLIB_EXPORT urlToLocalFile(const QString & url);
QString SHAREDLIB_EXPORT urlToLocalFile(const QVariant & value);

QString SHAREDLIB_EXPORT findExistingFile(const QString &filePath, const QString &dir);
QString SHAREDLIB_EXPORT resolveFilePath(const QString &filePath, const QString &dir);

QString SHAREDLIB_EXPORT standardDesktopLocation();
QString SHAREDLIB_EXPORT standardAppDataLocation();
QString SHAREDLIB_EXPORT standardTempLocation();

QString SHAREDLIB_EXPORT buildFilterString(const FileFormatsList &filters);
QString SHAREDLIB_EXPORT collectAllExtensions(const FileFormatsList &filters);
QString SHAREDLIB_EXPORT getOpenFileName(QWidget *parent, const QString &title, const QString &defaultExt, const QString &directory, const FileFormatsList &filters);
QString SHAREDLIB_EXPORT getSaveFileName(QWidget *parent, const QString &title, const QString &defaultExt, const QString &fileName, const FileFormatsList &filters);
