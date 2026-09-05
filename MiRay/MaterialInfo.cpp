#include "MaterialInfo.h"
#include "MainWindow.h"
#include "PreviewRenderer.h"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QQmlContext>
#include <QQuickItem>
#include <QDebug>

#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Utils/FileUtils.h"

enum {
	DataRole = Qt::UserRole + 1,
	TypeRole,
	SourceObjectRole,
	NameRole,
};

static const QString SETTINGS_LAST_IOR_FILE = "lastIorFile";
static const QString SETTINGS_LAST_MATERIAL_PATH = "lastMaterialPath";

static void findGeometries(Selection & selection, ISceneElement * node, const IMaterial * material)
{
	for (size_t i = 0, count = node->numChildren(SceneElement_All); i < count; i++) {
		auto child = node->child(i, SceneElement_All);
		if (child->type() & SceneElement_Geometry) {
			if (static_cast<IGeometry *>(child)->material() == material)
				selection.insert(child);
		} else {
			findGeometries(selection, child, material);
		}
	}
}

// ------------------------------------------------------------------------ //

MaterialProxy::MaterialProxy(IMaterial & material)
	: m_material(material)
	, m_name("Material Name")
	, m_enabled("Enabled")
	, m_bump("Bump", "Material Bump Map")
	, m_doubleSided("Double-sided")
	, m_medium("Medium")
	, m_iorN("Refractive Index N")
	, m_absorptionColor("Absorption")
	, m_absorptionAttenuation("Attenuation")
	, m_priority("Priority")
	, m_emissionColor("Emission Color")
	, m_emissionScale("Emission Scale")
	, m_subsurfaceScattering("Subsurface Scattering")
	, m_scatteringColor("Scattering Color")
	, m_scatteringScale("Scattering Scale")
	, m_scatteringAsymmetry("Scattering Asymmetry")
{
	m_name.setParam(&material.name());
	m_bump.setParam(&material.bump());
	m_doubleSided.setParam(&material.doubleSided());
	m_medium.setParam(&material.medium());
	m_iorN.setParam(&material.indexOfRefraction().n());
	m_absorptionColor.setParam(&material.absorptionColor());
	m_absorptionAttenuation.setParam(&material.absorptionAttenuation());
	m_priority.setParam(&material.priority());
	m_emissionColor.setParam(&material.emissionColor());
	m_emissionScale.setParam(&material.emissionScale());
	m_subsurfaceScattering.setParam(&material.subsurfaceScattering());
	m_scatteringColor.setParam(&material.scatteringColor());
	m_scatteringScale.setParam(&material.scatteringScale());
	m_scatteringAsymmetry.setParam(&material.scatteringAsymmetry());
}

QVariant MaterialProxy::data()
{
	QVariantMap map;
	map["type"] = 1;
	map["name"] = QVariant::fromValue(static_cast<QObject *>(&m_name));
	map["enabled"] = QVariant::fromValue(static_cast<QObject *>(&m_enabled));
	map["bump"] = QVariant::fromValue(static_cast<QObject *>(&m_bump));
	map["doubleSided"] = QVariant::fromValue(static_cast<QObject *>(&m_doubleSided));
	map["medium"] = QVariant::fromValue(static_cast<QObject *>(&m_medium));
	map["iorN"] = QVariant::fromValue(static_cast<QObject *>(&m_iorN));
	map["absorptionColor"] = QVariant::fromValue(static_cast<QObject *>(&m_absorptionColor));
	map["absorptionAttenuation"] = QVariant::fromValue(static_cast<QObject *>(&m_absorptionAttenuation));
	map["priority"] = QVariant::fromValue(static_cast<QObject *>(&m_priority));
	map["emissionColor"] = QVariant::fromValue(static_cast<QObject *>(&m_emissionColor));
	map["emissionScale"] = QVariant::fromValue(static_cast<QObject *>(&m_emissionScale));
	map["subsurfaceScattering"] = QVariant::fromValue(static_cast<QObject *>(&m_subsurfaceScattering));
	map["scatteringColor"] = QVariant::fromValue(static_cast<QObject *>(&m_scatteringColor));
	map["scatteringScale"] = QVariant::fromValue(static_cast<QObject *>(&m_scatteringScale));
	map["scatteringAsymmetry"] = QVariant::fromValue(static_cast<QObject *>(&m_scatteringAsymmetry));
	return map;
}

// ------------------------------------------------------------------------ //

MaterialGroupProxy::MaterialGroupProxy(IMaterialGroup & group)
	: m_group(group)
	, m_name("Group Name")
	, m_enabled("Enabled")
	, m_mask("Mask", "Group Mask")
{
	m_name.setParam(&group.name());
	m_enabled.setParam(&group.enabled());
	m_mask.setParam(&group.mask());
}

QVariant MaterialGroupProxy::data()
{
	QVariantMap map;
	map["type"] = 2;
	map["name"] = QVariant::fromValue(static_cast<QObject *>(&m_name));
	map["enabled"] = QVariant::fromValue(static_cast<QObject *>(&m_enabled));
	map["mask"] = QVariant::fromValue(static_cast<QObject *>(&m_mask));
	return map;
}

// ------------------------------------------------------------------------ //

MaterialLayerProxy::MaterialLayerProxy(IMaterialLayer & layer)
	: m_layer(layer)
	, m_name("Layer Name")
	, m_enabled("Enabled")
	, m_mask("Mask", "Layer Mask")
	, m_bump("Bump", "Layer Bump Map")
	, m_diffuseLayer("Diffuse Layer")
	, m_diffuseColor("Color", "Diffuse Map")
	, m_diffuseOpacity("Opacity", "Opacity Map")
	, m_diffuseTransmission("Transmission", "Transmission Map")
	, m_diffuseTextureLayerMask("Transparency is Mask")
	, m_emissiveLayer("Emissive Layer")
	, m_emissiveColor("Color", "Emissive Map")
	, m_emissiveIntensity("Intensity")
	, m_specularLayer("Specular Layer")
	, m_iorType("Refraction Type")
	, m_iorN("Refractive Index N")
	, m_iorK("Extinction Coefficient K")
	, m_iorFilename("IOR File", MainWindow::instance()->settings(), SETTINGS_LAST_IOR_FILE)
	, m_reflection("Reflection", "Reflection Map")
	, m_transmission("Transmission", "Transmission Map")
	, m_reflection90Level("Reflection 90 Level")
	, m_reflection90("Reflection 90", "Reflection 90 Map")
	, m_roughness("Roughness", "Roughness Map")
	, m_anisotropy("Anisotropy", "Anisotropy Map")
	, m_anisotropyAngle("Angle", "Anisotropy Angle Map")
	, m_thinFilmInterference("Thin-Film Interference")
	, m_thickness("Thickness", "Thickness Map")
	, m_minThickness("Min Thickness")
	, m_filmIorType("Film Refraction Type")
	, m_filmIorN("Film Refractive Index N")
	, m_filmIorK("Film Extinction Coefficient K")
	, m_filmIorFilename("Film IOR File", MainWindow::instance()->settings(), SETTINGS_LAST_IOR_FILE)
{
	m_name.setParam(&layer.name());
	m_enabled.setParam(&layer.enabled());
	m_mask.setParam(&layer.mask());
	m_bump.setParam(&layer.bump());
	m_diffuseLayer.setParam(&layer.diffuseLayer());
	m_diffuseColor.setParam(&layer.diffuseColor());
	m_diffuseOpacity.setParam(&layer.diffuseOpacity());
	m_diffuseTransmission.setParam(&layer.diffuseTransmission());
	m_diffuseTextureLayerMask.setParam(&layer.diffuseTextureLayerMask());
	m_emissiveLayer.setParam(&layer.emissiveLayer());
	m_emissiveColor.setParam(&layer.emissiveColor());
	m_emissiveIntensity.setParam(&layer.emissiveIntensity());
	m_specularLayer.setParam(&layer.specularLayer());
	m_iorType.setParam(&layer.indexOfRefraction().type());
	m_iorN.setParam(&layer.indexOfRefraction().n());
	m_iorK.setParam(&layer.indexOfRefraction().k());
	m_iorFilename.setParam(&layer.indexOfRefraction().fileName());
	m_reflection.setParam(&layer.reflection());
	m_transmission.setParam(&layer.transmission());
	m_reflection90Level.setParam(&layer.reflection90Level());
	m_reflection90.setParam(&layer.reflection90());
	m_roughness.setParam(&layer.roughness());
	m_anisotropy.setParam(&layer.anisotropy());
	m_anisotropyAngle.setParam(&layer.anisotropyAngle());
	m_thinFilmInterference.setParam(&layer.thinFilmInterference());
	m_thickness.setParam(&layer.thickness());
	m_minThickness.setParam(&layer.minThickness());
	m_filmIorType.setParam(&layer.filmIndexOfRefraction().type());
	m_filmIorN.setParam(&layer.filmIndexOfRefraction().n());
	m_filmIorK.setParam(&layer.filmIndexOfRefraction().k());
	m_filmIorFilename.setParam(&layer.filmIndexOfRefraction().fileName());
}

QVariant MaterialLayerProxy::data()
{
	QVariantMap map;
	map["type"] = 3;
	map["name"] = QVariant::fromValue(static_cast<QObject *>(&m_name));
	map["enabled"] = QVariant::fromValue(static_cast<QObject *>(&m_enabled));
	map["mask"] = QVariant::fromValue(static_cast<QObject *>(&m_mask));
	map["bump"] = QVariant::fromValue(static_cast<QObject *>(&m_bump));
	map["diffuseLayer"] = QVariant::fromValue(static_cast<QObject *>(&m_diffuseLayer));
	map["diffuseColor"] = QVariant::fromValue(static_cast<QObject *>(&m_diffuseColor));
	map["diffuseOpacity"] = QVariant::fromValue(static_cast<QObject *>(&m_diffuseOpacity));
	map["diffuseTransmission"] = QVariant::fromValue(static_cast<QObject *>(&m_diffuseTransmission));
	map["diffuseTextureLayerMask"] = QVariant::fromValue(static_cast<QObject *>(&m_diffuseTextureLayerMask));
	map["emissiveLayer"] = QVariant::fromValue(static_cast<QObject *>(&m_emissiveLayer));
	map["emissiveColor"] = QVariant::fromValue(static_cast<QObject *>(&m_emissiveColor));
	map["emissiveIntensity"] = QVariant::fromValue(static_cast<QObject *>(&m_emissiveIntensity));
	map["specularLayer"] = QVariant::fromValue(static_cast<QObject *>(&m_specularLayer));
	map["iorType"] = QVariant::fromValue(static_cast<QObject *>(&m_iorType));
	map["iorN"] = QVariant::fromValue(static_cast<QObject *>(&m_iorN));
	map["iorK"] = QVariant::fromValue(static_cast<QObject *>(&m_iorK));
	map["iorFilename"] = QVariant::fromValue(static_cast<QObject *>(&m_iorFilename));
	map["reflection"] = QVariant::fromValue(static_cast<QObject *>(&m_reflection));
	map["transmission"] = QVariant::fromValue(static_cast<QObject *>(&m_transmission));
	map["reflection90Level"] = QVariant::fromValue(static_cast<QObject *>(&m_reflection90Level));
	map["reflection90"] = QVariant::fromValue(static_cast<QObject *>(&m_reflection90));
	map["roughness"] = QVariant::fromValue(static_cast<QObject *>(&m_roughness));
	map["anisotropy"] = QVariant::fromValue(static_cast<QObject *>(&m_anisotropy));
	map["anisotropyAngle"] = QVariant::fromValue(static_cast<QObject *>(&m_anisotropyAngle));
	map["thinFilmInterference"] = QVariant::fromValue(static_cast<QObject *>(&m_thinFilmInterference));
	map["thickness"] = QVariant::fromValue(static_cast<QObject *>(&m_thickness));
	map["minThickness"] = QVariant::fromValue(static_cast<QObject *>(&m_minThickness));
	map["filmIorType"] = QVariant::fromValue(static_cast<QObject *>(&m_filmIorType));
	map["filmIorN"] = QVariant::fromValue(static_cast<QObject *>(&m_filmIorN));
	map["filmIorK"] = QVariant::fromValue(static_cast<QObject *>(&m_filmIorK));
	map["filmIorFilename"] = QVariant::fromValue(static_cast<QObject *>(&m_filmIorFilename));
	return map;
}

// ------------------------------------------------------------------------ //

MaterialInfo::MaterialInfo(IScene & scene, PreviewRenderer * previewRenderer)
	: m_scene(scene)
	, m_manager(scene.materialManager())
	, m_qmlContext(nullptr)
	, m_quickItem(nullptr)
	, m_material(nullptr)
	, m_editObject(nullptr)
	, m_previewRenderer(previewRenderer)
	, m_materialIndex(-1)
	, m_layerIndex(-1)
	, m_treeModel(new QStandardItemModel(this))
	, m_materialsList(new QStandardItemModel(this))
{
	QHash<int, QByteArray> roleNames;
	roleNames[DataRole] = "data";
	roleNames[TypeRole] = "type";
	roleNames[SourceObjectRole] = "node";
	m_treeModel->setItemRoleNames(roleNames);

	roleNames.clear();
	roleNames[NameRole] = "name";
	m_materialsList->setItemRoleNames(roleNames);

	connect(&m_manager, SIGNAL(materialsListChanged()), this, SLOT(onMaterialsListChanged()));
	connect(&m_manager, SIGNAL(materialChanged(const IMaterial *, bool)), this, SLOT(onMaterialChanged(const IMaterial *, bool)));

	if (m_previewRenderer) {
		connect(m_previewRenderer, SIGNAL(rendered(const IMaterial *)), this, SLOT(onPreviewRendered(const IMaterial *)));
		connect(m_previewRenderer, SIGNAL(progressChanged()), this, SIGNAL(previewProgressChanged()));
	}

	onMaterialsListChanged();
	setMaterial(nullptr);
}

MaterialInfo::~MaterialInfo()
{
	disconnect(&m_manager, SIGNAL(materialsListChanged()), this, SLOT(onMaterialsListChanged()));
	disconnect(&m_manager, SIGNAL(materialChanged(const IMaterial *, bool)), this, SLOT(onMaterialChanged(const IMaterial *, bool)));
}

void MaterialInfo::setQmlContext(QQmlContext * qmlContext, QQuickItem * quickItem)
{
	if (m_qmlContext == qmlContext)
		return;

	if (m_previewRenderer) {
		m_previewRenderer->stop();
	}

	if (m_qmlContext)
	{
		m_qmlContext->setContextProperty("materialsList", nullptr);
		m_qmlContext->setContextProperty("material", nullptr);
		m_qmlContext->setContextProperty("materialPresenter", nullptr);
	}

	m_qmlContext = qmlContext;
	m_quickItem = quickItem;

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("materialsList", m_materialsList.data());
		m_qmlContext->setContextProperty("material", m_treeModel.data());
		m_qmlContext->setContextProperty("materialPresenter", this);

		QString thumbnail;
		if (m_material && m_material->getPreview().bits()) {
			thumbnail = QString("image://materialpreview/preview?t=%1").arg(QDateTime::currentMSecsSinceEpoch());
		}
		m_qmlContext->setContextProperty("materialThumbnail", thumbnail.isEmpty() ? QVariant() : thumbnail);

		updateCurrentIndex();
		updateTreeModel();
	}
}

void MaterialInfo::setMaterial(IMaterial * material)
{
	if (m_material == material) {
		return;
	}

	m_material = material;
	m_editObject = nullptr;
	updateCurrentIndex();
	updateTreeModel();

	QString thumbnail;
	if (material && material->getPreview().bits()) {
		thumbnail = QString("image://materialpreview/preview?t=%1").arg(QDateTime::currentMSecsSinceEpoch());
	}

	if (m_previewRenderer) {
		m_previewRenderer->stop();
	}

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("materialThumbnail", thumbnail.isEmpty() ? QVariant() : thumbnail);
	}
}

void MaterialInfo::onMaterialChanged(const IMaterial * material, bool completelyChanged)
{
	if (m_material == material && completelyChanged) {
		updateTreeModel();
	}
}

void MaterialInfo::onMaterialsListChanged()
{
	m_materialsList->clear();
	m_mlProxyObjects.clear();

	for (int i = 0, count = m_manager.count(); i < count; i++) {
		auto material = m_manager.get(i);
		auto materialItem = new QStandardItem();
		auto materialProxy = std::make_unique<StringParamProxy>("Name");
		materialProxy->setParam(&material->name());
		materialItem->setData(QVariant::fromValue(static_cast<QObject *>(materialProxy.get())), NameRole);
		materialItem->setData(QVariant::fromValue(static_cast<QObject *>(material)), SourceObjectRole);
		m_mlProxyObjects.emplace_back(std::move(materialProxy));
		m_materialsList->appendRow(materialItem);
	}

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("materialsList", m_materialsList.data());
	}

	m_materialIndex = getCurrentIndex();
	if (m_materialIndex < 0 && !m_mlProxyObjects.empty()) {
		m_materialIndex = 0;
	}
	setMaterial((m_materialIndex >= 0 && m_materialIndex < (int)m_manager.count()) ? m_manager.get(m_materialIndex) : nullptr);
}

int MaterialInfo::getCurrentIndex() const
{
	for (int i = 0, count = m_manager.count(); i < count; i++) {
		if (m_material == m_manager.get(i)) {
			return i;
		}
	}
	return -1;
}

void MaterialInfo::updateCurrentIndex()
{
	m_materialIndex = getCurrentIndex();
	if (m_quickItem) {
		QVariant retVal;
		QMetaObject::invokeMethod(m_quickItem, "setMaterial", Q_RETURN_ARG(QVariant, retVal), Q_ARG(QVariant, m_materialIndex));
	}
}

void MaterialInfo::updateTreeModel()
{
	m_treeModel->clear();
	m_proxyObjects.clear();

	if (m_material) {
		QModelIndex active;
		auto materialItem = new QStandardItem();
		auto materialProxy = std::make_unique<MaterialProxy>(*m_material);
		materialItem->setData(materialProxy->data(), DataRole);
		materialItem->setData(1, TypeRole);
		materialItem->setData(QVariant::fromValue(static_cast<QObject *>(m_material)), SourceObjectRole);
		m_treeModel->appendRow(materialItem);
		m_proxyObjects.emplace_back(std::move(materialProxy));

		for (size_t gi = 0; gi < m_material->numGroups(); gi++) {
			auto * group = m_material->group(gi);
			auto groupProxy = std::make_unique<MaterialGroupProxy>(*group);
			auto groupItem = new QStandardItem();
			groupItem->setData(groupProxy->data(), DataRole);
			groupItem->setData(2, TypeRole);
			groupItem->setData(QVariant::fromValue(static_cast<QObject *>(group)), SourceObjectRole);
			materialItem->appendRow(groupItem);
			m_proxyObjects.emplace_back(std::move(groupProxy));

			if (m_editObject == group) {
				active = m_treeModel->indexFromItem(groupItem);
			}

			for (size_t li = 0; li < group->numLayers(); li++) {
				auto * layer = group->layer(li);
				auto layerProxy = std::make_unique<MaterialLayerProxy>(*layer);
				auto layerItem = new QStandardItem();
				layerItem->setData(layerProxy->data(), DataRole);
				layerItem->setData(3, TypeRole);
				layerItem->setData(QVariant::fromValue(static_cast<QObject *>(layer)), SourceObjectRole);
				groupItem->appendRow(layerItem);
				m_proxyObjects.emplace_back(std::move(layerProxy));

				if (m_editObject == layer || (m_editObject == nullptr && !active.isValid())) {
					m_editObject = layer;
					active = m_treeModel->indexFromItem(layerItem);
				}
			}
		}

		if (!active.isValid()) {
			active = m_treeModel->indexFromItem(materialItem);
		}

		if (m_quickItem) {
			QVariant retVal;
			QMetaObject::invokeMethod(m_quickItem, "expandTree", Q_RETURN_ARG(QVariant, retVal), Q_ARG(QVariant, active));
		}
	}

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("material", m_treeModel.data());
	}
}

void MaterialInfo::createMaterial()
{
	setMaterial(m_manager.create("Untitled"));
}

void MaterialInfo::deleteMaterial()
{
	if (m_material) {
		auto index = std::max(getCurrentIndex() - 1, 0);
		m_manager.remove({ m_material });
		if (index < m_manager.count())
			setMaterial(m_manager.get(index));
	}
}

void MaterialInfo::cloneMaterial()
{
	if (m_material) {
		auto data = m_material->save(ModelSavingContext());
		m_manager.load(data, ModelLoadingContext(), getCurrentIndex() + 1);
	}
}

void MaterialInfo::saveMaterial()
{
	if (m_material) {
		FileFormatsList filters = { { "MiRay Material", { "mirayMaterial" } } };
		QString lastPath = MainWindow::instance()->settings().value(SETTINGS_LAST_MATERIAL_PATH).toString();
		QString defaultName = sanitizeFilename(m_material->name().get()) + ".mirayMaterial";
		QString defaultPath = lastPath.isEmpty() ? QDir(standardDesktopLocation()).filePath(defaultName) : QFileInfo(lastPath).absoluteDir().filePath(defaultName);
		QString filePath = getSaveFileName(nullptr, "Save Material", "mirayMaterial", defaultPath, filters);
		if (!filePath.isEmpty()) {
			auto data = m_material->save(ModelSavingContext());
			writeFile(filePath, data);
			MainWindow::instance()->settings().setValue(SETTINGS_LAST_MATERIAL_PATH, filePath);
		}
	}
}

void MaterialInfo::selectGeometry()
{
	if (m_material) {
		Selection selection;
		findGeometries(selection, &m_scene.root(), m_material);
		m_scene.setSelection(selection, SelectionOperation_Set);
	}
}

void MaterialInfo::applyMaterial()
{
	if (m_material) {
		m_scene.setMaterial(m_scene.geomSelection(), m_material);
	}
}

void MaterialInfo::removeUnusedMaterials()
{
	m_manager.removeUnusedMaterials();
}

void MaterialInfo::addMaterial(QList<QUrl> urls)
{
	for (const auto & url : urls) {
		auto filePath = QDir::toNativeSeparators(url.isLocalFile() ? url.toLocalFile() : url.toString());
		auto data = readFile(filePath);
		if (!data.isEmpty()) {
			if (auto material = m_manager.load(data, ModelLoadingContext(), m_manager.count())) {
				setMaterial(material);
			}
		}
	}
}

bool MaterialInfo::canDropUrls(QList<QUrl> urls, int index)
{
	for (const auto & url : urls) {
		auto filePath = QDir::toNativeSeparators(url.isLocalFile() ? url.toLocalFile() : url.toString());
		if (filePath.startsWith("miray://material/")) {
			return true;
		}

		QFileInfo fileInfo(filePath);
		auto suffix = fileInfo.suffix();
		if (!suffix.compare("mirayMaterial", Qt::CaseInsensitive)) {
			return true;
		}

		if (m_manager.isIORFile(filePath)) {
			return true;
		}
	}
	return false;
}

void MaterialInfo::dropUrls(QList<QUrl> urls, int index)
{
	index = qBound(0, index, m_manager.count());

	for (const auto & url : urls) {
		IMaterial * material = nullptr;
		auto filePath = QDir::toNativeSeparators(url.isLocalFile() ? url.toLocalFile() : url.toString());
		if (filePath.startsWith("miray://material/")) {
			filePath.remove(0, QString("miray://material/").length());
			material = m_manager.getByName(filePath);
			if (material) {
				m_manager.move(material, index <= getCurrentIndex() ? index : index - 1);
			}
		} else {
			QFileInfo fileInfo(filePath);
			auto suffix = fileInfo.suffix();
			if (!suffix.compare("mirayMaterial", Qt::CaseInsensitive)) {
				const auto data = readFile(filePath);
				if (!data.isEmpty()) {
					material = m_manager.load(data, ModelLoadingContext(), index);
				}
			} else if (m_manager.isIORFile(filePath)) {
				material = m_manager.createFromIORFile(filePath);
				if (material) {
					m_manager.move(material, index);
				}
			}
		}

		if (material) {
			setMaterial(material);
		}
	}
}

void MaterialInfo::setEditIndex(QModelIndex index)
{
	auto item = m_treeModel->itemFromIndex(index);
	m_editObject = item ? qvariant_cast<QObject *>(item->data(SourceObjectRole)) : nullptr;
}

void MaterialInfo::editAdd(QModelIndex index)
{
	auto item = m_treeModel->itemFromIndex(index);
	if (!m_material || !item) {
		return;
	}

	auto object = qvariant_cast<QObject *>(item->data(SourceObjectRole));

	if (m_material == object) {
		m_material->addGroup(m_material->numGroups());
		return;
	}

	for (size_t gi = 0; gi < m_material->numGroups(); gi++) {
		auto * group = m_material->group(gi);
		if (group == object) {
			group->addLayer(group->numLayers(), QByteArray(), ModelLoadingContext());
			return;
		}

		for (size_t li = 0; li < group->numLayers(); li++) {
			auto * layer = group->layer(li);
			if (layer == object) {
				group->addLayer(li + 1, QByteArray(), ModelLoadingContext());
				return;
			}
		}
	}
}

void MaterialInfo::editDelete(QModelIndex index)
{
	auto item = m_treeModel->itemFromIndex(index);
	if (!m_material || !item) {
		return;
	}

	auto object = qvariant_cast<QObject *>(item->data(SourceObjectRole));

	for (size_t gi = 0; gi < m_material->numGroups(); gi++) {
		auto * group = m_material->group(gi);
		if (group == object) {
			m_editObject = m_material;
			m_material->removeGroup(gi);
			return;
		}

		for (size_t li = 0; li < group->numLayers(); li++) {
			auto * layer = group->layer(li);
			if (layer == object) {
				m_editObject = group;
				group->removeLayer(li);
				return;
			}
		}
	}
}

void MaterialInfo::editMoveUp(QModelIndex index)
{
	auto item = m_treeModel->itemFromIndex(index);
	if (!m_material || !item) {
		return;
	}

	auto object = qvariant_cast<QObject *>(item->data(SourceObjectRole));

	for (size_t gi = 0; gi < m_material->numGroups(); gi++) {
		auto * group = m_material->group(gi);
		if (group == object) {
			if (gi > 0) {
				m_material->moveGroup(gi, gi - 1);
			}
			return;
		}

		for (size_t li = 0; li < group->numLayers(); li++) {
			auto * layer = group->layer(li);
			if (layer == object) {
				if (li > 0) {
					group->moveLayer(li, li - 1);
				}
				return;
			}
		}
	}
}

void MaterialInfo::editMoveDown(QModelIndex index)
{
	auto item = m_treeModel->itemFromIndex(index);
	if (!m_material || !item) {
		return;
	}

	auto object = qvariant_cast<QObject *>(item->data(SourceObjectRole));

	for (size_t gi = 0; gi < m_material->numGroups(); gi++) {
		auto * group = m_material->group(gi);
		if (group == object) {
			if (gi < m_material->numGroups() - 1) {
				m_material->moveGroup(gi, gi + 1);
			}
			return;
		}

		for (size_t li = 0; li < group->numLayers(); li++) {
			auto * layer = group->layer(li);
			if (layer == object) {
				if (li < group->numLayers() - 1) {
					group->moveLayer(li, li + 1);
				}
				return;
			}
		}
	}
}

void MaterialInfo::setMaterial(int index)
{
	if (index >= 0 && index < m_manager.count()) {
		setMaterial(m_manager.get(index));
	}
}

void MaterialInfo::updateThumbnail()
{
	if (m_material && m_previewRenderer) {
		m_previewRenderer->setMaterial(m_material);
		m_previewRenderer->renderStep();
	}
}

void MaterialInfo::setPreviewModel(int index)
{
	if (m_previewRenderer && index >= 0 && index < m_previewRenderer->getScenes().size()) {
		m_previewRenderer->setScene(m_previewRenderer->getScenes().at(index));
		if (m_material) {
			m_previewRenderer->setMaterial(m_material);
			m_previewRenderer->renderStep();
		}
	}
}

QStringList MaterialInfo::getPreviewModels() const
{
	return m_previewRenderer ? m_previewRenderer->getScenes() : QStringList();
}

void MaterialInfo::onPreviewRendered(const IMaterial * material)
{
	if (m_material == material) {
		QString thumbnail;
		if (m_material && m_material->getPreview().bits()) {
			thumbnail = QString("image://materialpreview/preview?t=%1").arg(QDateTime::currentMSecsSinceEpoch());
		}
		if (m_qmlContext) {
			m_qmlContext->setContextProperty("materialThumbnail", thumbnail.isEmpty() ? QVariant() : thumbnail);
		}

		if (m_previewRenderer) {
			m_previewRenderer->renderStep();
		}
	}
}

QImage MaterialInfo::getCurrentMaterialPreview() const
{
	if (m_material) {
		return m_material->getPreview();
	}
	return QImage();
}

float MaterialInfo::previewProgress() const
{
	// qDebug() << "previewProgress" << (m_previewRenderer ? m_previewRenderer->progress() : 1.0f);
	return m_previewRenderer ? m_previewRenderer->progress() : 1.0f;
}
