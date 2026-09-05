#include "AssetResolver.h"
#include "../Shared/Utils/FileUtils.h"

AssetResolver::AssetResolver(const QString& sourceFolder)
	: m_sourceFolder(sourceFolder)
{
}

QString AssetResolver::normalizePath(QString path)
{
	path.replace('\\', '/');
	return path;
}

void AssetResolver::ensureSubfolderIndex() const
{
	if (m_indexed)
		return;

	m_indexed = true;
	if (m_sourceFolder.isEmpty())
		return;

	QDir dir(m_sourceFolder);
	if (!dir.exists())
		return;

	QDirIterator it(m_sourceFolder, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		QString fullPath = normalizePath(it.next());
		QString baseName = QFileInfo(fullPath).fileName().toLower();
		if (m_fileNameCache.find(baseName.toStdString()) == m_fileNameCache.end()) {
			m_fileNameCache[baseName.toStdString()] = fullPath;
		}
	}
}

QString AssetResolver::resolveAsset(const QString& rawPath) const
{
	if (rawPath.isEmpty())
		return QString();

	// try optimistic approach
	QString optimistic = resolveFilePath(rawPath, m_sourceFolder);
	if (QFileInfo(optimistic).exists())
		return normalizePath(optimistic);

	// try direct match in source folder
	QString fileName = QFileInfo(rawPath).fileName();
	if (!m_sourceFolder.isEmpty() && !fileName.isEmpty()) {
		QString directCombined = normalizePath(QDir(m_sourceFolder).filePath(fileName));
		if (QFileInfo(directCombined).exists())
			return directCombined;

		// lazy index subfolders once and lookup in O(1)
		ensureSubfolderIndex();
		auto it = m_fileNameCache.find(fileName.toLower().toStdString());
		if (it != m_fileNameCache.end())
			return it->second;
	}

	// fallback to optimistic name
	return normalizePath(optimistic);
}
