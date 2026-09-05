/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#include "MaterialManager.h"
#include "IORReader.h"
#include "../Samplers/Halton.h"

#include "../../Shared/Interfaces/SerializationContext.h"

// ------------------------------------------------------------------------ //

MaterialManager::MaterialManager(Scene & scene)
	: m_core(scene.core())
	, m_scene(scene)
	, m_colorCount(0)
	, m_defaultMaterial(new MaterialImpl(*this, ""))
{
	m_defaultMaterial->_setGuid(QUuid());
}

MaterialManager::~MaterialManager()
{
}

// ------------------------------------------------------------------------ //

void MaterialManager::reset()
{
	m_materials.clear();
}

// ------------------------------------------------------------------------ //

class CreateMaterialCommand : public QUndoCommand
{
	MaterialManager & m_manager;
	MaterialPtr m_material;
	size_t	m_position;

	void redo()
	{
		m_manager._add(m_material, m_position);
	}

	void undo()
	{
		m_material = m_manager._remove(m_position);
	}

public:
	CreateMaterialCommand(MaterialManager & manager, MaterialPtr material, size_t position)
		: m_manager(manager)
		, m_material(std::move(material))
		, m_position(position)
	{
		m_material->_setGuid(QUuid::createUuid()); // generate new guid for the new material
	}
};

class RemoveMaterialCommand : public QUndoCommand
{
	MaterialManager & m_manager;
	MaterialPtr m_material;
	size_t m_index;

	void redo()
	{
		m_material = m_manager._remove(m_index);
	}

	void undo()
	{
		m_manager._add(m_material, m_index);
	}

public:
	RemoveMaterialCommand(MaterialManager & manager, MaterialImpl * material)
		: m_manager(manager)
	{
		auto & mats = m_manager.materials();
		auto it = std::find_if(mats.begin(), mats.end(), [material](const MaterialPtr &item) { return item.get() == material; });
		assert(it != mats.end());
		m_index = std::distance(mats.begin(), it);
	}
};

class MoveMaterialCommand : public QUndoCommand
{
	MaterialManager &	m_manager;
	size_t				m_from, m_to;

	void redo() override
	{
		auto material = m_manager._remove(m_from);
		m_manager._add(material, m_to);
	}

	void undo() override
	{
		auto material = m_manager._remove(m_to);
		m_manager._add(material, m_from);
	}

public:
	MoveMaterialCommand(MaterialManager & manager, size_t from, size_t to)
		: m_manager(manager)
		, m_from(from)
		, m_to(to)
	{
	}
};

// ------------------------------------------------------------------------ //

void MaterialManager::_add(MaterialPtr & material, size_t pos)
{
	assert(pos <= m_materials.size());
	if (pos < m_materials.size())
		m_materials.insert(m_materials.begin() + pos, std::move(material));
	else
		m_materials.push_back(std::move(material));
	fireMaterialListChanged();
}

MaterialPtr MaterialManager::_remove(size_t pos)
{
	assert(pos < m_materials.size());
	auto material = std::move(m_materials[pos]);
	m_materials.erase(m_materials.begin() + pos);
	fireMaterialListChanged();
	return material;
}

// ------------------------------------------------------------------------ //

MaterialImpl* MaterialManager::_create(const QString &name, const QJsonObject &params, const ModelLoadingContext &ctx)
{
	MaterialPtr material(new MaterialImpl(*this, name));
	if (!material->_loadParams(params, ctx)) {
		return nullptr;
	}

	// here the same guid means the same material
	auto id = material->_guid();
	auto it = std::find_if(m_materials.begin(), m_materials.end(), [&id](const MaterialPtr &p) { return p->_guid() == id; });
	if (it != m_materials.end()) {
		return it->get();
	}

	m_materials.emplace_back(std::move(material));
	return m_materials.back().get();
}

MaterialImpl* MaterialManager::_create(const QString &name, const QDomElement &elem, const ModelLoadingContext &ctx)
{
	MaterialPtr material(new MaterialImpl(*this, name));
	if (!material->_load(elem, ctx)) {
		return nullptr;
	}

	// here the same guid means the same material
	auto id = material->_guid();
	auto it = std::find_if(m_materials.begin(), m_materials.end(), [&id](const MaterialPtr &p) { return p->_guid() == id; });
	if (it != m_materials.end()) {
		return it->get();
	}

	m_materials.emplace_back(std::move(material));
	return m_materials.back().get();
}

MaterialImpl* MaterialManager::create(const QString &name)
{
	auto material = new MaterialImpl(*this, name);
	m_scene.pushCommand(new CreateMaterialCommand(*this, MaterialPtr(material), m_materials.size()));
	return material;
}

MaterialImpl* MaterialManager::load(const QByteArray &data, const ModelLoadingContext &ctx, size_t position)
{
	assert(position <= m_materials.size());

	auto material = new MaterialImpl(*this, QString());
	material->load(data, ctx);
	m_scene.pushCommand(new CreateMaterialCommand(*this, MaterialPtr(material), position));
	return material;
}

void MaterialManager::remove(const MaterialSelection & materials)
{
	if (materials.empty()) return;
	m_scene.undoStack().beginMacro("Remove materials");
	for (auto material : materials) {
		m_scene.replaceMaterial(material, m_defaultMaterial.get());
		m_scene.pushCommand(new RemoveMaterialCommand(*this, static_cast<MaterialImpl *>(material)));
	}
	m_scene.undoStack().endMacro();
}

void MaterialManager::move(IMaterial *material, size_t pos)
{
	auto it = std::find_if(m_materials.begin(), m_materials.end(), [material](const MaterialPtr &item) { return item.get() == material; });
	if (it == m_materials.end())
		return;

	auto from = (size_t)std::distance(m_materials.begin(), it);
	if (from == pos || pos >= m_materials.size())
		return;

	m_scene.pushCommand(new MoveMaterialCommand(*this, from, pos));
}

void MaterialManager::removeUnusedMaterials()
{
	MaterialSelection materials;
	for (const auto & material : m_materials) {
		if (!m_scene.isMaterialUsed(material.get()))
				materials.insert(material.get());
	}

	if (!materials.empty())
		remove(materials);
}

// ------------------------------------------------------------------------ //

MaterialImpl *MaterialManager::get(int pos) const
{
	assert(pos >= 0 && pos < (int)m_materials.size());
	if (pos < 0 || pos >= (int)m_materials.size())
		throw std::out_of_range(QString("Invalid material index %1, there are just %2 materials in the list").arg(pos).arg(m_materials.size()).toStdString());

	return m_materials[pos].get();
}

bool MaterialManager::isDefault(const IMaterial* material) const
{
	return m_defaultMaterial.get() == material;
}

MaterialImpl * MaterialManager::getByName(const QString & name) const
{
	for (const auto & material : m_materials) {
		if (material->name().get().compare(name) == 0)
			return material.get();
	}

	return nullptr;
}

// ------------------------------------------------------------------------ //

bool MaterialManager::isValidName(const QString & name) const
{
	return getByName(name) == nullptr;
}

QString MaterialManager::getValidName(const QString & name) const
{
	if (isValidName(name))
		return name;

	int index = 0;
	while (true) {
		QString newName = name + QString().asprintf(" %d", ++index);
		if (isValidName(newName))
			return newName;
	}

	return name;
}

// ------------------------------------------------------------------------ //

vec3 MaterialManager::generateUniqueColor()
{
	HaltonSampler sampler;
	sampler.setResolution(2, 2);
	sampler.init(0, 0, ++m_colorCount);
	return sampler.generate3D();
}

// ------------------------------------------------------------------------ //

void MaterialManager::fireMaterialListChanged()
{
	emit materialsListChanged();
}

void MaterialManager::fireMaterialChanged(const IMaterial * material, bool completelyChanged)
{
	emit materialChanged(material, completelyChanged);
}

// ------------------------------------------------------------------------ //

bool MaterialManager::isIORFile(const QString &filename) const
{
	return ior::isIORFile(filename);
}

IMaterial *MaterialManager::createFromIORFile(const QString &filename)
{
	auto name = QFileInfo(filename).completeBaseName();
	auto mtl = new MaterialImpl(*this, name);
	assert(mtl && mtl->numGroups() == 1);
	auto group = mtl->group(0);
	assert(group && group->numLayers() == 1);
	auto layer = group->layer(0);
	assert(layer);

	mtl->indexOfRefraction().fileName()._set(filename);
	mtl->medium()._setIndex(MEDIUM_TYPE_MEASURED);
	mtl->fireChanged(PID_MATERIAL_MEDIUM_TYPE);

	layer->diffuseLayer()._set(false);
	layer->specularLayer()._set(true);
	layer->indexOfRefraction().type()._setIndex((int)IORType::MEASURED);
	layer->indexOfRefraction().fileName()._set(filename);
	layer->indexOfRefraction().fireChanged(PID_IOR_FILENAME);
	layer->fireChanged(PID_LAYER);

	m_scene.pushCommand(new CreateMaterialCommand(*this, MaterialPtr(mtl), m_materials.size()));

	return mtl;
}

// ------------------------------------------------------------------------ //

class ReplaceMaterialCommand : public QUndoCommand
{
	Scene &			m_scene;
	MaterialImpl &	m_material;
	QString			m_name;
	QByteArray		m_newData;
	QByteArray		m_oldData;
	ModelLoadingContext m_ctx;

	void redo()
	{
		m_scene.lock(SceneInternalModification_Material);
		m_material.load(m_newData, m_ctx);
		m_material.name()._set(m_name); // restore name
		m_material.fireChanged(PID_MATERIAL);
		m_scene.unlock(SceneInternalModification_Material);
	}

	void undo()
	{
		m_scene.lock(SceneInternalModification_Material);
		m_material.load(m_oldData, ModelLoadingContext());
		m_material.name()._set(m_name); // restore name
		m_material.fireChanged(PID_MATERIAL);
		m_scene.unlock(SceneInternalModification_Material);
	}

public:
	ReplaceMaterialCommand(Scene & scene, MaterialImpl & material, const QByteArray & data, const ModelLoadingContext &ctx)
		: m_scene(scene)
		, m_material(material)
		, m_name(material.name().get())
		, m_newData(data)
		, m_oldData(material.save(ModelSavingContext()))
		, m_ctx(ctx)
	{
	}

	~ReplaceMaterialCommand()
	{
	}
};

void MaterialManager::replaceWithSerializedOne(IMaterial * material, const QByteArray & data, const ModelLoadingContext &ctx)
{
	m_scene.pushCommand(new ReplaceMaterialCommand(m_scene, *static_cast<MaterialImpl *>(material), data, ctx));
}

void MaterialManager::collectFileNames(QSet<QString> &res) const
{
	for (auto &m : m_materials)
		m->collectFileNames(res);
}

void MaterialManager::updateFileNames(const QMap<QString, QString>& map)
{
	for (auto &m : m_materials)
		m->updateFileNames(map);
}
