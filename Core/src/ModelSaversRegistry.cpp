#include "ModelSaversRegistry.h"
#include "../../Shared/Interfaces/ObjectsFactory.h"

ModelSaversRegistry::ModelSaversRegistry(IObjectsFactory &f)
	: m_factory(f)
{
}

ModelSaversRegistry::~ModelSaversRegistry()
{
}

void ModelSaversRegistry::registerModelSaver(const ModelSavingFormat & format, bool exporter)
{
	(exporter ? m_formatsExport : m_formatsSave).append(format);
	m_savingFilters.push_back({format.description, format.extensions});
}

const ListOfModelSavingFormats &ModelSaversRegistry::allFormats(bool exporters) const
{
	return exporters ? m_formatsExport : m_formatsSave;
}

const FileFormatsList & ModelSaversRegistry::savingFilters() const
{
	return m_savingFilters;
}

const ModelSavingFormat *ModelSaversRegistry::findFormatByExtension(const QString & ext) const
{
	const ListOfModelSavingFormats * formatsLists[] = { &m_formatsSave, &m_formatsExport };

	for (auto formats : formatsLists) {
		for (auto & format : *formats) {
			if (format.extensions.contains(ext, Qt::CaseInsensitive))
				return &format;
		}
	}

	return nullptr;
}

bool ModelSaversRegistry::canSaveFile(const QString & fileName) const
{
	const auto ext = QFileInfo(fileName).suffix();
	return findFormatByExtension(ext) != nullptr;
}

std::unique_ptr<IModelSaver> ModelSaversRegistry::getSaver(const QString & fileName) const
{
	const auto ext = QFileInfo(fileName).suffix();
	const auto *format = findFormatByExtension(ext);
	if (!format)
		throw std::runtime_error("The format is unknown, can't find a saver for it.");

	auto obj = m_factory.createByName(format->id);
	auto saver = qobject_cast<IModelSaver *>(obj.data());
	if (!saver) {
		throw std::runtime_error(QString("Can't cast model saver object (id = '%1') to IModelSaver interface").arg(format->id).toStdString());
	}

	obj.take();
	return std::unique_ptr<IModelSaver>(saver);
}
