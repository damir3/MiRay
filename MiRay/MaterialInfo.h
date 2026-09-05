#pragma once

#include "ParamProxy.h"
#include <QStandardItem>
#include <QStandardItemModel>

class IMaterial;
class IMaterialGroup;
class IMaterialLayer;
class IMaterialManager;
class IScene;
class QQmlContext;
class QQuickItem;
class PreviewRenderer;

class MaterialProxy : public QObject
{
	Q_OBJECT

	IMaterial & m_material;

	StringParamProxy  m_name;
	BooleanParamProxy m_enabled; // dummy param

	ScalarParamProxy  m_bump;
	BooleanParamProxy m_doubleSided;

	EnumParamProxy    m_medium;
	ScalarParamProxy  m_iorN;

	ColorParamProxy   m_absorptionColor;
	ScalarParamProxy  m_absorptionAttenuation;

	IntegerParamProxy m_priority;

	ColorParamProxy   m_emissionColor;
	ScalarParamProxy  m_emissionScale;

	BooleanParamProxy m_subsurfaceScattering;
	ColorParamProxy   m_scatteringColor;
	ScalarParamProxy  m_scatteringScale;
	ScalarParamProxy  m_scatteringAsymmetry;

public:
	MaterialProxy(IMaterial & material);
	QVariant data();
};

class MaterialGroupProxy : public QObject
{
	Q_OBJECT

	IMaterialGroup & m_group;

	StringParamProxy  m_name;
	BooleanParamProxy m_enabled;
	ScalarParamProxy  m_mask;

public:
	MaterialGroupProxy(IMaterialGroup & group);
	QVariant data();
};

class MaterialLayerProxy : public QObject
{
	Q_OBJECT

	IMaterialLayer & m_layer;

	StringParamProxy  m_name;
	BooleanParamProxy m_enabled;

	ScalarParamProxy  m_mask;
	ScalarParamProxy  m_bump;

	BooleanParamProxy m_diffuseLayer;
	ColorParamProxy   m_diffuseColor;
	ScalarParamProxy  m_diffuseOpacity;
	ColorParamProxy   m_diffuseTransmission;
	BooleanParamProxy m_diffuseTextureLayerMask;

	BooleanParamProxy m_emissiveLayer;
	ColorParamProxy   m_emissiveColor;
	ScalarParamProxy  m_emissiveIntensity;

	BooleanParamProxy m_specularLayer;
	EnumParamProxy    m_iorType;
	ScalarParamProxy  m_iorN;
	ScalarParamProxy  m_iorK;
	FileNameParamProxy m_iorFilename;
	ColorParamProxy   m_reflection;
	ColorParamProxy   m_transmission;
	ScalarParamProxy  m_reflection90Level;
	ColorParamProxy   m_reflection90;
	ScalarParamProxy  m_roughness;
	ScalarParamProxy  m_anisotropy;
	ScalarParamProxy  m_anisotropyAngle;

	BooleanParamProxy m_thinFilmInterference;
	ScalarParamProxy  m_thickness;
	ScalarParamProxy  m_minThickness;
	EnumParamProxy    m_filmIorType;
	ScalarParamProxy  m_filmIorN;
	ScalarParamProxy  m_filmIorK;
	FileNameParamProxy m_filmIorFilename;

public:
	MaterialLayerProxy(IMaterialLayer & layer);
	QVariant data();
};

class MaterialInfo : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int materialIndex MEMBER m_materialIndex)
	Q_PROPERTY(int layerIndex MEMBER m_layerIndex)
	Q_PROPERTY(float previewProgress READ previewProgress NOTIFY previewProgressChanged)

	IScene & m_scene;
	IMaterialManager & m_manager;
	QQmlContext * m_qmlContext;
	QQuickItem * m_quickItem;
	IMaterial * m_material;
	QObject * m_editObject;
	PreviewRenderer * m_previewRenderer;

	int m_materialIndex;
	int m_layerIndex;

	std::vector<std::unique_ptr<QObject>> m_proxyObjects;
	QScopedPointer<QStandardItemModel> m_treeModel;

	std::vector<std::unique_ptr<QObject>> m_mlProxyObjects;
	QScopedPointer<QStandardItemModel> m_materialsList;

	int getCurrentIndex() const;
	void updateCurrentIndex();
	void updateTreeModel();

private slots:
	void onPreviewRendered(const class IMaterial * material);

public:
	MaterialInfo(IScene & scene, PreviewRenderer * previewRenderer = nullptr);
	~MaterialInfo();

	void setQmlContext(QQmlContext *, QQuickItem *);
	void setMaterial(IMaterial *);

	QStandardItemModel * materialsList() const { return m_materialsList.data(); }
	QStandardItemModel * treeModel() const { return m_treeModel.data(); }
	class QImage getCurrentMaterialPreview() const;

	float previewProgress() const;

	Q_INVOKABLE void createMaterial();
	Q_INVOKABLE void deleteMaterial();
	Q_INVOKABLE void cloneMaterial();
	Q_INVOKABLE void saveMaterial();
	Q_INVOKABLE void selectGeometry();
	Q_INVOKABLE void applyMaterial();
	Q_INVOKABLE void removeUnusedMaterials();
	Q_INVOKABLE void addMaterial(QList<QUrl> urls);

	Q_INVOKABLE bool canDropUrls(QList<QUrl> urls, int index);
	Q_INVOKABLE void dropUrls(QList<QUrl> urls, int index);

	Q_INVOKABLE void updateThumbnail();
	Q_INVOKABLE void setPreviewModel(int index);
	Q_INVOKABLE QStringList getPreviewModels() const;

	Q_INVOKABLE void setEditIndex(QModelIndex index);
	Q_INVOKABLE void editAdd(QModelIndex index);
	Q_INVOKABLE void editDelete(QModelIndex index);
	Q_INVOKABLE void editMoveUp(QModelIndex index);
	Q_INVOKABLE void editMoveDown(QModelIndex index);

	Q_INVOKABLE void setMaterial(int index);

signals:
	void previewProgressChanged();

public slots:
	void onMaterialsListChanged();
	void onMaterialChanged(const IMaterial * material, bool completelyChanged);
};
