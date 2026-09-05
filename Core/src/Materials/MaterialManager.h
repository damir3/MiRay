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

#pragma once

#include "MaterialImpl.h"

class Scene;

class MaterialManager final : public IMaterialManager
{
	Q_OBJECT

	CoreInstance & m_core;
	Scene &        m_scene;
	int            m_colorCount;
	MaterialsList  m_materials;
	MaterialPtr    m_defaultMaterial;

public:
	MaterialManager(Scene & scene);
	~MaterialManager();

	void reset();

	CoreInstance & core() const { return m_core; }
	Scene & scene() const { return m_scene; }
	MaterialImpl * getDefault() const { return m_defaultMaterial.get(); }

	MaterialImpl * _create(const QString & name, const QJsonObject & params, const ModelLoadingContext & ctx);
	MaterialImpl * _create(const QString & name, const QDomElement & data, const ModelLoadingContext & ctx);
	MaterialImpl * create(const QString & name) override;
	MaterialImpl * load(const QByteArray & data, const ModelLoadingContext & ctx, size_t position) override;
	void remove(const MaterialSelection & materials) override;
	void move(IMaterial * material, size_t pos) override;
	void removeUnusedMaterials() override;

	void replaceWithSerializedOne(IMaterial * material, const QByteArray & data, const ModelLoadingContext & ctx) override;

	void _add(MaterialPtr & material, size_t pos);
	MaterialPtr _remove(size_t pos);

	void collectFileNames(QSet<QString> & res) const;
	void updateFileNames(const QMap<QString, QString> &);

	int count() const override { return (int)m_materials.size(); }
	MaterialImpl * get(int pos) const override;
	bool isDefault(const IMaterial *) const override;
	MaterialsList & materials() { return m_materials; }

	MaterialImpl * getByName(const QString & name) const override;

	bool isValidName(const QString & name) const;
	QString getValidName(const QString & name) const;

	bool isIORFile(const QString & filename) const override;
	IMaterial *createFromIORFile(const QString & filename) override;

	vec3 generateUniqueColor();

	void fireMaterialListChanged();
	void fireMaterialChanged(const IMaterial * material, bool completelyChanged);
};
