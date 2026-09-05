#pragma once

#include "../../Shared/Interfaces/ModelSaver.h"

class IObjectsFactory;

class ModelSaversRegistry : public IModelSaverRegistry
{
	ListOfModelSavingFormats m_formatsSave;
	ListOfModelSavingFormats m_formatsExport;
	FileFormatsList m_savingFilters;

	IObjectsFactory & m_factory;

	const ModelSavingFormat *findFormatByExtension(const QString &ext) const;

public:
	ModelSaversRegistry(IObjectsFactory &f);
	~ModelSaversRegistry() override;

	void registerModelSaver(const ModelSavingFormat &fmt, bool exporter) override;
	const ListOfModelSavingFormats &allFormats(bool exporters) const override;
	const FileFormatsList & savingFilters() const override;
	bool canSaveFile(const QString &fileName) const override;
	std::unique_ptr<IModelSaver> getSaver(const QString &fileName) const override;
};
