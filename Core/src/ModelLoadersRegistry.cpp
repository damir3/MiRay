#include "ModelLoadersRegistry.h"
#include "../../Shared/Interfaces/ObjectsFactory.h"
#include "../../Shared/Interfaces/SerializationContext.h"

ModelLoadersRegistry::ModelLoadersRegistry(IObjectsFactory &f)
	: m_factory(f)
{
}

ModelLoadersRegistry::~ModelLoadersRegistry()
{
}

void ModelLoadersRegistry::registerModelLoader(const ModelLoadingFormat & format)
{
	m_formats.append(format);
	m_loadingFilters.push_back({format.description, format.extensions});
}

const ListOfModelLoadingFormats &ModelLoadersRegistry::allFormats() const
{
	return m_formats;
}

const FileFormatsList & ModelLoadersRegistry::loadingFilters() const
{
	return m_loadingFilters;
}

bool ModelLoadersRegistry::canLoadFile(const QString &fileName, bool deeperCheck) const
{
	auto ext = QFileInfo(fileName).suffix().toLower();

	ModelLoadingContext ctx;
	ctx.sourceFileName = fileName;
	ctx.sourceFolder = nativePath(QFileInfo(fileName).absolutePath());
	ctx.loadNewScene = true;
	ctx.putOnTheFloor = true;

	for (auto & fmt : m_formats) {
		for (auto & e : fmt.extensions) {
			if (e.compare(ext, Qt::CaseInsensitive) == 0) {
				if (!deeperCheck)
					return true;

				auto obj = m_factory.createByName(fmt.id);
				auto loader = qobject_cast<IModelLoader *>(obj.data());
				if (!loader) {
					throw std::runtime_error(QString("Can't cast model loader object (id = '%1') to IModelLoader interface").arg(fmt.id).toStdString());
				}

				if (loader->canLoad(ctx))
					return true;
			}
		}
	}

	return false;
}

std::unique_ptr<IModelLoader> ModelLoadersRegistry::getLoader(const QString &fileName) const
{
	ModelLoadingContext ctx;
	ctx.sourceFileName = fileName;
	ctx.sourceFolder = nativePath(QFileInfo(fileName).absolutePath());
	ctx.loadNewScene = true;
	ctx.putOnTheFloor = true;

	for (auto & fmt : m_formats) {
		auto obj = m_factory.createByName(fmt.id);
		auto loader = qobject_cast<IModelLoader *>(obj.data());
		if (!loader) {
			throw std::runtime_error(QString("Can't cast model loader object (id = '%1') to IModelLoader interface").arg(fmt.id).toStdString());
		}

		if (loader->canLoad(ctx)) {
			obj.take();
			return std::unique_ptr<IModelLoader>(loader);
		}
	}

	throw std::runtime_error("The format is unknown, can't find a parser for it.");
}
