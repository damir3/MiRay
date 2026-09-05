#include "SceneTree.h"

#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Shared/Utils/FileUtils.h"

#include <QTreeView>
#include <QQmlContext>
#include <QDebug>
#include <QDir>

enum NodeRoles {
	NameRole = Qt::UserRole + 1,
	VisibleRole,
	MaterialRole,
	LevelRole,
	SelectedRole,
	ExpandedRole,
};

static const QString LOCAL_MATERIAL_PREFIX("miray://material/");
static const QString SCENE_TREE_PREFIX("miray://scenetree/");
static const QString MIME_TEXT_PLAIN("text/plain");
static const QString MIME_TEXT_URI_LIST("text/uri-list");
static const QString MIME_DRAGGED_ROWS("miray/dragged-rows");

/*
 TODO:
 - drop above and below nodes
 - popup menu:
 	- cut, copy, paste, delete
 	- extract meshes, combine meshes, group selected
 	- expand selected, expand selected and children
 	- collapse selected
 */

SceneTree::SceneTree(IScene * scene, QObject * parent)
	: QObject(parent)
	, m_scene(scene)
	, m_qmlContext(nullptr)
	, m_quickItem(nullptr)
	, m_treeModel(new SceneTreeModel(*scene))
	, m_filterModel(new NodeFilterProxyModel(this, m_treeModel.data()))
	, m_currentElement(nullptr)
{
	init();
}

SceneTree::~SceneTree()
{
}

void SceneTree::setQmlContext(QQmlContext * qmlContext, QQuickItem * quickItem)
{
	if (m_qmlContext == qmlContext)
		return;

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("sceneTreePresenter", nullptr);
		m_qmlContext->setContextProperty("sceneTreeModel", nullptr);
	}

	m_qmlContext = qmlContext;
	m_quickItem = quickItem;

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("sceneTreePresenter", this);
		m_qmlContext->setContextProperty("sceneTreeModel", m_filterModel.get());
	}
}

// ------------------------------------------------------------------------ //

NodeFilterProxyModel::NodeFilterProxyModel(SceneTree *parent, SceneTreeModel * sceneTreeModel)
	: QSortFilterProxyModel(parent)
	, m_sceneTreeModel(sceneTreeModel)
{
	setSourceModel(sceneTreeModel);
	setFilterCaseSensitivity(Qt::CaseInsensitive);
//	setRecursiveFilteringEnabled(true);
}

static bool filterElemRecursively(ISceneElement * elem, QRegExp & regExp)
{
	if (elem->name().contains(regExp))
		return true;

	for (size_t i = 0, count = elem->numChildren(SceneElement_AnyNode); i < count; i++) {
		if (filterElemRecursively(elem->child(i, SceneElement_AnyNode), regExp))
			return true;
	}
	return false;
}

//NodeItem * NodeFilterProxyModel::itemByRow(int row, const QModelIndex & sourceParent) const
//{
//	auto index = m_sceneTreeModel->index(row, 0, sourceParent);
//	return m_sceneTreeModel->itemByIndex(index);
//}

ISceneElement * NodeFilterProxyModel::nodeByRow(int row) const
{
	auto sourceIndex = mapToSource(index(row, 0));
	return m_sceneTreeModel->nodeByIndex(sourceIndex);
}

//int NodeFilterProxyModel::rowByNode(ISceneElement * elem) const
//{
//	auto index = m_sceneTreeModel->indexByNode(elem);
//	auto sourceIndex = mapFromSource(index);
//	return sourceIndex.row();
//}

bool NodeFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const
{
	auto index = m_sceneTreeModel->index(sourceRow, 0, sourceParent);
	auto item = m_sceneTreeModel->itemByIndex(index);
//	auto item = itemByRow(sourceRow, sourceParent);
	auto elem = &item->getElement();
	if (!elem)
		return false;
//	qDebug() << sourceRow << elem->name();

	if (elem->type() == SceneElement_Geometry)
		elem = elem->parent();

	auto regExp = filterRegExp();
	return item->isVisible() && filterElemRecursively(elem, regExp);
}

// ------------------------------------------------------------------------ //

void SceneTree::setFilter(QString string)
{
	m_filterModel->setFilterWildcard(string);
}

QVariantMap SceneTree::createDragMimeData(int row) const
{
	QStringList rows;
	QStringList names;
	auto selection = m_scene->nodeSelection();
	for (auto elem : selection) {
		if (elem->type() == SceneElement_Node ||
			elem->type() == SceneElement_MeshNode) {
			auto row = m_treeModel->rowByNode(elem);
			if (row >= 0) {
				rows << QString::number(row);
				names << elem->name().get();
			}
		}
	}

	QVariantMap mimeData;
	mimeData[MIME_TEXT_PLAIN] = names.join("; ");
	mimeData[MIME_DRAGGED_ROWS] = rows.join(',');
	//qDebug() << "createDragMimeData" << mimeData;

	return mimeData;
}

static int getNodeIndex(ISceneElement & elem)
{
	auto parent = elem.parent();
	for (size_t i = 0, n = parent->numChildren(SceneElement_Node); i < n; i++) {
		if (&elem == parent->child(i, SceneElement_Node))
			return (int)i;
	}
	return -1;
}

NodeItem * SceneTree::getItemByRow(int row) const
{
	auto sourceIndex = m_filterModel->mapToSource(m_filterModel->index(row, 0));
	return m_treeModel->itemByIndex(sourceIndex);
}

ISceneElement * SceneTree::getNodeByRow(int row) const
{
	auto sourceIndex = m_filterModel->mapToSource(m_filterModel->index(row, 0));
	return m_treeModel->nodeByIndex(sourceIndex);
}

std::pair<INode *, size_t> SceneTree::getDropTarget(int row, int delta) const
{
	auto res = std::make_pair((INode *)nullptr, (size_t)-1);

	if (row >= m_filterModel->rowCount()) {
		res.first = &m_scene->root();
		res.second = m_scene->root().numChildren(SceneElement_Node);
	} else if (auto item = getItemByRow(row)) {
		auto & elem = item->getElement();
		if (delta < 0) {// drop above
			auto parent = elem.parent();
			if (parent->type() == SceneElement_Node || parent->type() == SceneElement_MeshNode) {
				int i = getNodeIndex(elem);
				if (i >= 0) {
					res.first = parent;
					res.second = (size_t)i;
				}
			}
		} else if (delta > 0) {// drop below
			if (!item->isExpanded()) {
				auto parent = elem.parent();
				if (parent->type() == SceneElement_Node || parent->type() == SceneElement_MeshNode) {
					int i = getNodeIndex(elem);
					if (i >= 0) {
						res.first = parent;
						res.second = (size_t)i + 1;
					}
				}
			} else if (elem.type() == SceneElement_Node || elem.type() == SceneElement_MeshNode) {
				res.first = static_cast<INode *>(&elem);
				res.second = 0;
			}
		} else if (elem.type() == SceneElement_Node || elem.type() == SceneElement_MeshNode) {
			res.first = static_cast<INode *>(&elem);
		}

	}

	return res;
}

bool SceneTree::canDropMimeData(const QVariantMap & mime, const QUrl & url, int row, int delta)
{
	auto draggedRows = mime[MIME_DRAGGED_ROWS].toString();
	if (!draggedRows.isEmpty()) {
		auto target = getDropTarget(row, delta);
		auto & selection = m_scene->selection();
		// qDebug() << target.first << target.second;
		return target.first && selection.find(target.first) == selection.end();
	}

	if (!url.isEmpty()) {
		auto path = url.toString();
		if (path.startsWith(LOCAL_MATERIAL_PREFIX) || path.endsWith(".mirayMaterial") || m_scene->materialManager().isIORFile(path)) {
			if (auto item = getItemByRow(row)) {
				if (item->getElement().type() == SceneElement_Geometry)
					return true;
			}
		}
	}

	return false;
}

bool SceneTree::dropMimeData(const QVariantMap & mime, const QUrl & url, int row, int delta)
{
	qDebug() << "drop" << mime[MIME_TEXT_URI_LIST].toStringList() << url;

	auto draggedRows = mime[MIME_DRAGGED_ROWS].toString();
	if (!draggedRows.isEmpty()) {
		QStringList list = draggedRows.split(",", Qt::SkipEmptyParts);
		NodeSelection nodesToMove;
		for (auto & index : list) {
			auto elem = m_treeModel->nodeByIndex(m_treeModel->index(index.toInt()));
			if (elem->type() == SceneElement_Node || elem->type() == SceneElement_MeshNode)
				nodesToMove.push_back(static_cast<INode *>(elem));
		}

		auto target = getDropTarget(row, delta);
		auto & selection = m_scene->selection();
		if (target.first && selection.find(target.first) == selection.end()) {
			m_scene->moveNodes(nodesToMove, target.first, target.second, QString());
			return true;
		}
	}

	if (!url.isEmpty()) {
		if (auto item = getItemByRow(row)) {
			auto path = url.toString();
			auto & elem = item->getElement();
			if (elem.type() == SceneElement_Geometry) {
				auto & materialManager = m_scene->materialManager();
				IMaterial * material = nullptr;
				if (path.startsWith(LOCAL_MATERIAL_PREFIX)) {
					path.remove(0, LOCAL_MATERIAL_PREFIX.length());
					material = materialManager.getByName(path);
				} else {
					auto filePath = QDir::toNativeSeparators(url.isLocalFile() ? url.toLocalFile() : url.toString());
					// qDebug() << "filePath" << filePath;

					if (materialManager.isIORFile(filePath)) {
						material = materialManager.createFromIORFile(filePath);
					} else if (filePath.endsWith(".mirayMaterial")) {
						const auto data = readFile(filePath);
						if (!data.isEmpty())
							material = materialManager.load(data, ModelLoadingContext(), materialManager.count());
					}
				}
				if (material) {
					auto geometry = static_cast<IGeometry *>(&elem);
					auto selection = m_scene->geomSelection();
					if (std::find(selection.begin(), selection.end(), geometry) == selection.end())
						selection = { geometry };
					m_scene->setMaterial(selection, material);
					return true;
				}
			}
		}
	}

	return false;
}

void SceneTree::select(int row, int modifiers)
{
	//qDebug() << "select" << row;
	auto element = row >= 0 ? getNodeByRow(row) : nullptr;

	Selection selection;
	if (element) {
		if (modifiers & Qt::ControlModifier) {
			selection = m_scene->selection();
			if (selection.find(element) != selection.end())
				selection.erase(element);
			else
				selection.insert(element);
		} else if (modifiers & Qt::ShiftModifier) {
			selection = m_scene->selection();
			auto index = m_treeModel->indexByNode(m_currentElement);
			auto sourceIndex = m_filterModel->mapFromSource(index);
			int currentRow = sourceIndex.row();
			if (currentRow >= 0 && currentRow < m_filterModel->rowCount()) {
				int minRow = std::min(currentRow, row);
				int maxRow = std::max(currentRow, row);
				for (int i = minRow; i <= maxRow; i++) {
					if (auto element = getNodeByRow(i))
						selection.insert(element);
				}
			} else
				selection.insert(element);
		} else
			selection.insert(element);
	}

	m_currentElement = row >= 0 ? getNodeByRow(row) : nullptr;
	m_scene->setSelection(selection, SelectionOperation_Set);
}

void SceneTree::expandSelected(bool includeChildren)
{
	for (auto * node : m_scene->nodeSelection())
		m_treeModel->setExpanded(node, true, includeChildren);
}

void SceneTree::collapseSelected(bool includeChildren)
{
	for (auto * node : m_scene->nodeSelection())
		m_treeModel->setExpanded(node, false, includeChildren);
}

// ------------------------------------------------------------------------ //

NodeItem::NodeItem(SceneTreeModel & model, ISceneElement & element, int level)
	: m_model(model)
	, m_element(element)
	, m_nameParam(nullptr)
	, m_level(level)
	, m_sceneTreeVisible(level == 0)
	, m_expanded(false)
	, m_selected(false)
{
	updateMaterial();
}

NodeItem::~NodeItem()
{
	if (m_nameParam)
		disconnect(m_nameParam, SIGNAL(changed()), this, SLOT(onNameParamChanged()));
}

QVariant NodeItem::getData(int role) const
{
	if (m_element.type() & SceneElement_AnyNode) {
		auto & node = static_cast<INode &>(m_element);
		switch (role) {
			case VisibleRole: return node.visible().get();
			case NameRole: return node.name().get();
			case LevelRole: return m_level;
			case SelectedRole: return m_selected;
			case ExpandedRole: return node.numChildren(SceneElement_All) > 0 ? m_expanded : QVariant();
		}
	} else if (m_element.type() & SceneElement_Geometry) {
		auto & geometry = static_cast<IGeometry &>(m_element);
		switch (role) {
			case NameRole: return geometry.name();
			case MaterialRole: return geometry.material()->name().get();
			case LevelRole: return m_level;
			case SelectedRole: return m_selected;
		}
	}
	return QVariant();
}

bool NodeItem::setData(const QVariant & value, int role)
{
	if (m_element.type() & SceneElement_AnyNode) {
		auto & node = static_cast<INode &>(m_element);
		switch (role) {
			case VisibleRole: node.visible().set(value.toBool()); return true;
			case NameRole: node.name().set(value.toString()); return true;
			case ExpandedRole: m_expanded = value.toBool(); m_model.updateChildrenVisibility(node, m_expanded); return true;
			case SelectedRole: m_selected = value.toBool(); return true;
		}
	} else if (m_element.type() & SceneElement_Geometry) {
		auto & geometry = static_cast<IGeometry &>(m_element);
		switch (role) {
			case SelectedRole: m_selected = value.toBool(); return true;
		}
	}
	return false;
}

void NodeItem::updateMaterial()
{
	if (m_nameParam) {
		disconnect(m_nameParam, SIGNAL(changed()), this, SLOT(onNameParamChanged()));
		m_nameParam = nullptr;
	}

	if (m_element.type() & SceneElement_AnyNode) {
		auto & node = static_cast<INode &>(m_element);
		m_nameParam = &node.name();
	} else if (m_element.type() & SceneElement_Geometry) {
		auto & geometry = static_cast<IGeometry &>(m_element);
		m_nameParam = &geometry.material()->name();
	}

	if (m_nameParam)
		connect(m_nameParam, SIGNAL(changed()), this, SLOT(onNameParamChanged()));
}

void NodeItem::onNameParamChanged()
{
	m_model.onMaterialNameChanged(static_cast<const IGeometry *>(&m_element));
}

// ------------------------------------------------------------------------ //

SceneTreeModel::SceneTreeModel(IScene & scene)
	: m_scene(scene)
{
	connect(&m_scene, SIGNAL(completelyChanged()), this, SLOT(onSceneCompletelyChanged()));
	connect(&m_scene, SIGNAL(nodeChanged(const INode *, eNodeChanged)), this, SLOT(onNodeChanged(const INode *, eNodeChanged)));
	connect(&m_scene, SIGNAL(afterSceneUpdate(const SceneModification &)), this, SLOT(onAfterSceneUpdate(const SceneModification &)));
	connect(&m_scene, SIGNAL(selectionChanged()), this, SLOT(onSceneSelectionChanged()));
	onSceneCompletelyChanged();
}

SceneTreeModel::~SceneTreeModel()
{
	disconnect(&m_scene, SIGNAL(completelyChanged()), this, SLOT(onSceneCompletelyChanged()));
	disconnect(&m_scene, SIGNAL(nodeChanged(const INode *, eNodeChanged)), this, SLOT(onNodeChanged(const INode *, eNodeChanged)));
	disconnect(&m_scene, SIGNAL(afterSceneUpdate(const SceneModification &)), this, SLOT(onAfterSceneUpdate(const SceneModification &)));
	disconnect(&m_scene, SIGNAL(selectionChanged()), this, SLOT(onSceneSelectionChanged()));
}

void SceneTreeModel::onSceneCompletelyChanged()
{
	beginResetModel();

	std::vector<NodeItem *> nodes;
	std::map<const ISceneElement *, int> nodeRow;
	auto & root = m_scene.root();
	for (size_t i = 0, n = root.numChildren(SceneElement_All); i < n; i++)
		updateTreeItem(root.child(i, SceneElement_All), 0, true, nodes, nodeRow);

	for (auto * nodeItem : m_nodes) {
		if (nodeItem)
			delete nodeItem;
	}

	m_nodes = std::move(nodes);
	m_nodeRow = std::move(nodeRow);

	// for (size_t i = 0; i < m_nodes.size(); i++)
	// 	qDebug() << i << m_nodes[i]->getElement().name();

	endResetModel();
}

void SceneTreeModel::onNodeChanged(const INode * node, eNodeChanged what)
{
//	qDebug() << "onNodeChanged" << node->name() << what;
	if (what & NodeChanged_ChildrenList) {
		onSceneCompletelyChanged();
	} else if (what & NodeChanged_Visibility) {
		auto it = m_nodeRow.find(node);
		if (it != m_nodeRow.end())
			emit dataChanged(index(it->second, 0), index(it->second, 0));
	}
}

void SceneTreeModel::onAfterSceneUpdate(const SceneModification & mod)
{
	if (mod.type == SceneModification_Material) {
		if (mod.sourceElement && mod.sourceElement->type() == SceneElement_Geometry) {
			auto it = m_nodeRow.find(mod.sourceElement);
			if (it != m_nodeRow.end()) {
				m_nodes[it->second]->updateMaterial();
				emit dataChanged(index(it->second, 0), index(it->second, 0));
			}
		}
	}
}

void SceneTreeModel::onMaterialNameChanged(const IGeometry * geometry)
{
	auto it = m_nodeRow.find(geometry);
	if (it != m_nodeRow.end()) {
		m_nodes[it->second]->updateMaterial();
		emit dataChanged(index(it->second, 0), index(it->second, 0));
	}
}

void SceneTreeModel::onSceneSelectionChanged()
{
	auto newSelection = m_scene.selection();
	for (auto * node : m_selection) {
		if (newSelection.find(node) == newSelection.end()) {
			auto it = m_nodeRow.find(node);
			if (it != m_nodeRow.end())
				setData(index(it->second, 0), QVariant(false), SelectedRole);
		}
	}

	for (auto * node : newSelection) {
		if (m_selection.find(node) == m_selection.end()) {
			auto it = m_nodeRow.find(node);
			if (it != m_nodeRow.end())
				setData(index(it->second, 0), QVariant(true), SelectedRole);

			auto * parent = node->parent();
			while (parent) {
				auto it = m_nodeRow.find(parent);
				if (it != m_nodeRow.end())
					setData(index(it->second, 0), QVariant(true), ExpandedRole);

				parent = parent->parent();
			}
		}
	}

	m_selection = std::move(newSelection);
}

void SceneTreeModel::setExpanded(INode * node, bool expanded, bool recursive)
{
	auto it = m_nodeRow.find(node);
	if (it != m_nodeRow.end())
		setData(index(it->second, 0), QVariant(expanded), ExpandedRole);

	if (recursive) {
		for (size_t i = 0, n = node->numChildren(SceneElement_Node); i < n; i++)
			setExpanded(static_cast<INode *>(node->child(i, SceneElement_Node)), expanded, recursive);
	}
}

void SceneTreeModel::updateTreeItem(ISceneElement * node, int level, bool visible, std::vector<NodeItem *> & nodes, std::map<const ISceneElement *, int> & nodeRow)
{
	NodeItem * nodeItem = nullptr;
	auto it = m_nodeRow.find(node);
	if (it != m_nodeRow.end()) {
		nodeItem = m_nodes[it->second];
		m_nodes[it->second] = nullptr;
	}

	if (!nodeItem) {
		nodeItem = new NodeItem(*this, *node, level);
		nodeItem->setSelected(m_selection.find(node) != m_selection.end());
	}

	nodeItem->setLevel(level);
	nodeItem->setVisible(visible);
	visible &= nodeItem->isExpanded();

	nodeRow[node] = (int)nodes.size();
	nodes.push_back(nodeItem);

	level++;
	for (size_t i = 0; i < node->numChildren(SceneElement_All); i++)
		updateTreeItem(node->child(i, SceneElement_All), level, visible, nodes, nodeRow);
}

int SceneTreeModel::rowCount(const QModelIndex &parent) const
{
	return (int)m_nodes.size();
}

QHash<int, QByteArray> SceneTreeModel::roleNames() const
{
	QHash<int, QByteArray> roleNames;
	roleNames[VisibleRole] = "visible";
	roleNames[NameRole] = "name";
	roleNames[LevelRole] = "level";
	roleNames[MaterialRole] = "material";
	roleNames[ExpandedRole] = "expanded";
	roleNames[SelectedRole] = "selected";
	return roleNames;
}

NodeItem * SceneTreeModel::itemByIndex(const QModelIndex &index) const
{
	if (index.isValid()) {
		const auto row = index.row();
		if (row >= 0 && row < (int)m_nodes.size())
			return m_nodes[row];
	}
	return nullptr;
}

ISceneElement * SceneTreeModel::nodeByIndex(const QModelIndex &index) const
{
	if (auto item = itemByIndex(index))
		return &item->getElement();
	return nullptr;
}

int SceneTreeModel::rowByNode(ISceneElement * element) const
{
	auto it = m_nodeRow.find(element);
	return it != m_nodeRow.end() ? it->second : -1;
}

QModelIndex SceneTreeModel::indexByNode(ISceneElement * element) const
{
	auto it = m_nodeRow.find(element);
	return it != m_nodeRow.end() ? index(it->second, 0) : QModelIndex();
}

void SceneTreeModel::updateChildrenVisibility(INode & node, bool expanded)
{
	for (size_t i = 0, count = node.numChildren(SceneElement_All); i < count; i++) {
		auto child = node.child(i, SceneElement_All);
		auto it = m_nodeRow.find(child);
		if (it != m_nodeRow.end() && it->second >= 0 && it->second < (int)m_nodes.size()) {
			auto nodeItem = m_nodes[it->second];
			if (nodeItem) {
				nodeItem->setVisible(expanded);
				emit dataChanged(index(it->second, 0), index(it->second, 0));

				if (child->type() & SceneElement_AnyNode)
					updateChildrenVisibility(static_cast<INode &>(*child), expanded & nodeItem->isExpanded());
			}
		}
	}
}

QVariant SceneTreeModel::data(const QModelIndex &index, int role) const
{
	if (auto item = itemByIndex(index))
		return item->getData(role);

	return QVariant();
}

bool SceneTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (auto item = itemByIndex(index)) {
		if (item->setData(value, role)) {
			emit dataChanged(index, index);
			return true;
		}
	}

	return false;
}
