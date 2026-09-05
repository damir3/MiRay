#pragma once

#include <QSortFilterProxyModel>

#include "../Shared/Interfaces/Scene.h"

class QItemSelectionModel;
class QItemSelection;

class ISceneElement;
class INode;
class IGeometry;

class SceneTreeModel;

class NodeItem : public QObject
{
	Q_OBJECT

	SceneTreeModel & m_model;
	ISceneElement &	m_element;
	IStringParameter *	m_nameParam;
	int m_level;
	bool m_sceneTreeVisible;
	bool m_expanded;
	bool m_selected;

public:
	NodeItem(SceneTreeModel & model, ISceneElement & element, int level);
	~NodeItem();

	ISceneElement &	getElement() const { return m_element; }
	void setLevel(int level) { m_level = level; }
	bool isVisible() const { return m_sceneTreeVisible; }
	void setVisible(bool v) { m_sceneTreeVisible = v; }
	bool isExpanded() const { return m_expanded; }
	bool isSelected() const { return m_selected; }
	void setSelected(bool selected) { m_selected = selected; }

	QVariant getData(int role) const;
	bool setData(const QVariant & value, int role);

	void updateMaterial();

public slots:
	void onNameParamChanged();
};

class SceneTreeModel : public QAbstractListModel
{
	Q_OBJECT

	IScene &	m_scene;

	std::vector<NodeItem *>	m_nodes;
	std::map<const ISceneElement *, int>	m_nodeRow;
	Selection 				m_selection;

	int rowCount(const QModelIndex &parent) const override;
	QHash<int, QByteArray> roleNames() const override;
	QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

	void updateTreeItem(ISceneElement * node, int level, bool visible, std::vector<NodeItem *> & nodes, std::map<const ISceneElement *, int> & nodeRow);

public:
	SceneTreeModel(IScene & scene);
	~SceneTreeModel();

	NodeItem * itemByIndex(const QModelIndex &index) const;
	ISceneElement * nodeByIndex(const QModelIndex &index) const;
	int rowByNode(ISceneElement *) const;
	QModelIndex indexByNode(ISceneElement *) const;

	void updateChildrenVisibility(INode & node, bool expanded);
	void setExpanded(INode * node, bool expanded, bool recursive);

public slots:
	void onSceneCompletelyChanged();
	void onNodeChanged(const INode *, eNodeChanged);
	void onAfterSceneUpdate(const SceneModification &);
	void onSceneSelectionChanged();
	void onMaterialNameChanged(const IGeometry *);

signals:
	void visibilityChanged();
};

class NodeFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	SceneTreeModel * m_sceneTreeModel;

public:
	NodeFilterProxyModel(class SceneTree * parent, SceneTreeModel * sceneTreeModel);

//	NodeItem * itemByRow(int row, const QModelIndex & sourceParent) const;
	ISceneElement * nodeByRow(int row) const;
//	int rowByNode(ISceneElement *) const;

	bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const override;
};

class SceneTree : public QObject
{
	Q_OBJECT

	IScene * const  m_scene;
	QQmlContext * m_qmlContext;
	QQuickItem * m_quickItem;

	QScopedPointer<SceneTreeModel>    		m_treeModel;
	QScopedPointer<NodeFilterProxyModel>	m_filterModel;

	ISceneElement * m_currentElement;

	NodeItem * getItemByRow(int row) const;
	ISceneElement * getNodeByRow(int row) const;
	std::pair<INode *, size_t> getDropTarget(int row, int delta) const;

public:
	SceneTree(IScene * scene, QObject * parent = nullptr);
	~SceneTree();

	void init() {}

	QSortFilterProxyModel * filterModel() const { return m_filterModel.get(); }
	QItemSelectionModel * selectionModel() const { return nullptr; }

	void setQmlContext(QQmlContext *, QQuickItem *);

	Q_INVOKABLE void setFilter(QString string);
	Q_INVOKABLE QVariantMap createDragMimeData(int row) const;
	Q_INVOKABLE bool canDropMimeData(const QVariantMap & mime, const QUrl & url, int row, int delta);
	Q_INVOKABLE bool dropMimeData(const QVariantMap & mime, const QUrl & url, int row, int delta);
	Q_INVOKABLE void select(int row, int modifiers);
	Q_INVOKABLE void expandSelected(bool includeChildren);
	Q_INVOKABLE void collapseSelected(bool includeChildren);
};
