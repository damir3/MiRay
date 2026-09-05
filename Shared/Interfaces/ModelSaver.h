#pragma once

#include "../Utils/FileUtils.h"

class IScene;
class INode;

struct ModelSavingFormat {
	QString	id;
	QString	description;
	QStringList	extensions; // just extensions, no dots
};

class SHAREDLIB_EXPORT IModelSaver : public QObject
{
	Q_OBJECT

public:
	virtual ~IModelSaver() {}

	virtual bool save(const ModelSavingContext &ctx, IScene *scene) = 0;
	virtual QByteArray exportNodes(const std::vector<INode *> &nodes) = 0;

signals:
	void updateProgress(float progress);
};

typedef QList<ModelSavingFormat>	ListOfModelSavingFormats;

class SHAREDLIB_EXPORT IModelSaverRegistry
{
protected:
	virtual ~IModelSaverRegistry() {}

public:
	virtual void registerModelSaver(const ModelSavingFormat &fmt, bool exporter) = 0;

	virtual const ListOfModelSavingFormats &allFormats(bool exporters) const = 0;

	virtual const FileFormatsList & savingFilters() const = 0;

	virtual bool canSaveFile(const QString &fileName) const = 0;

	virtual std::unique_ptr<IModelSaver> getSaver(const QString &fileName) const = 0;
};
