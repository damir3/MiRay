#pragma once

#include "../../Shared/Interfaces/ModelLoader.h"

class IObjectsFactory;

class ModelLoadersRegistry : public IModelLoaderRegistry
{
	ListOfModelLoadingFormats m_formats;
	FileFormatsList m_loadingFilters;

	IObjectsFactory & m_factory;

public:
	ModelLoadersRegistry(IObjectsFactory &f);
	~ModelLoadersRegistry() override;

	void registerModelLoader(const ModelLoadingFormat &fmt) override;
	const ListOfModelLoadingFormats &allFormats() const override;
	const FileFormatsList & loadingFilters() const override;
	bool canLoadFile(const QString &fileName, bool deeperCheck) const override;
	std::unique_ptr<IModelLoader> getLoader(const QString &fileName) const override;
};
