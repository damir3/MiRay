#pragma once

class AssetResolver {
public:
	explicit AssetResolver(const QString& sourceFolder);

	QString resolveAsset(const QString& rawPath) const;

	static QString normalizePath(QString path);

private:
	void ensureSubfolderIndex() const;

	QString m_sourceFolder;
	mutable bool m_indexed = false;
	mutable std::unordered_map<std::string, QString> m_fileNameCache;
};
