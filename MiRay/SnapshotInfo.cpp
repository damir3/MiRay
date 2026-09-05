#include "SnapshotInfo.h"

#include "../Shared/Interfaces/Snapshot.h"
#include "../Shared/Interfaces/SnapshotManager.h"

#include <QBuffer>
#include <QHash>
#include <QQmlContext>

enum { DataRole = Qt::UserRole + 1 };
static const QString LOCAL_SNAPSHOT_PREFIX("miray://snapshot/");

SnapshotInfo::SnapshotInfo(ISnapshotManager & manager)
	: m_manager(manager)
	, m_snapshot(nullptr)
	, m_currentIndex(-1)
	, m_qmlContext(nullptr)
	, m_quickItem(nullptr)
	, m_snapshotsList(new QStandardItemModel(this)) {
	connect(&m_manager, SIGNAL(changed()), this, SLOT(onSnapshotListChanged()));

	QHash<int, QByteArray> roleNames;
	roleNames.clear();
	roleNames[DataRole] = "snapshot";
	m_snapshotsList->setItemRoleNames(roleNames);

	onSnapshotListChanged();
}

SnapshotInfo::~SnapshotInfo() {
	disconnect(&m_manager, SIGNAL(changed()), this, SLOT(onSnapshotListChanged()));
}

void SnapshotInfo::setQmlContext(QQmlContext * qmlContext, QQuickItem * quickItem) {
	if (m_qmlContext == qmlContext) {
		return;
	}

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("snapshotsList", nullptr);
		m_qmlContext->setContextProperty("snapshotPresenter", nullptr);
	}

	m_qmlContext = qmlContext;
	m_quickItem = quickItem;

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("snapshotsList", m_snapshotsList.data());
		m_qmlContext->setContextProperty("snapshotPresenter", this);
	}
}

void SnapshotInfo::setSnapshot(ISnapshot * snapshot) {
	if (m_snapshot != snapshot) {
		m_snapshot = snapshot;
		updateCurrentIndex();
	}
}

int SnapshotInfo::getCurrentIndex() const {
	for (int i = 0, count = (int)m_manager.count(); i < count; i++) {
		if (m_snapshot == m_manager.get(i)) {
			return i;
		}
	}
	return -1;
}

void SnapshotInfo::updateCurrentIndex() {
	if (m_quickItem) {
		m_currentIndex = getCurrentIndex();
		QVariant retVal;
		QMetaObject::invokeMethod(m_quickItem, "setSnapshot", Q_RETURN_ARG(QVariant, retVal), Q_ARG(QVariant, m_currentIndex));
	}
}

void SnapshotInfo::setSnapshot(int index) {
	if (index >= 0 && index < (int)m_manager.count()) {
		setSnapshot(m_manager.get(index));
	}
}

void SnapshotInfo::onSnapshotListChanged() {
	m_snapshotsList->clear();
	m_proxyObjects.clear();

	for (size_t i = 0, count = m_manager.count(); i < count; i++) {
		auto snapshot = m_manager.get(i);
		auto snapshotProxy = std::make_unique<SnapshotProxy>(*snapshot);
		auto snapshotItem = new QStandardItem();
		snapshotItem->setData(snapshotProxy->modelData(), DataRole);
		m_proxyObjects.emplace_back(std::move(snapshotProxy));
		m_snapshotsList->appendRow(snapshotItem);
	}

	if (m_qmlContext) {
		m_qmlContext->setContextProperty("snapshotsList", m_snapshotsList.data());
	}

	m_currentIndex = getCurrentIndex();
	if (m_currentIndex < 0 && !m_proxyObjects.empty()) {
		m_currentIndex = 0;
	}
	m_snapshot = (m_currentIndex >= 0 && m_currentIndex < (int)m_manager.count()) ? m_manager.get(m_currentIndex) : nullptr;
	updateCurrentIndex();
}

void SnapshotInfo::createSnapshot() {
	m_snapshot = m_manager.create();
	onSnapshotListChanged();
}

void SnapshotInfo::deleteSnapshot() {
	if (m_snapshot) {
		m_manager.remove(m_snapshot);
	}
}

void SnapshotInfo::updateSnapshot() {
	if (m_snapshot) {
		m_snapshot->updateWithCurrent();
		if (m_currentIndex >= 0 && m_currentIndex < (int)m_proxyObjects.size()) {
			m_proxyObjects[m_currentIndex]->updatePreview();
			m_snapshotsList->item(m_currentIndex)->setData(m_proxyObjects[m_currentIndex]->modelData(), DataRole);
		}
	}
}

void SnapshotInfo::activateSnapshot() {
	if (m_snapshot) {
		m_snapshot->activate();
	}
}

bool SnapshotInfo::canDropUrls(QList<QUrl> urls, int index) {
	for (const auto & url : urls) {
		if (url.toString().startsWith(LOCAL_SNAPSHOT_PREFIX)) {
			return true;
		}
	}
	return false;
}

void SnapshotInfo::dropUrls(QList<QUrl> urls, int index) {
	int newIndex = qBound(0, index, (int)m_manager.count());

	for (const auto & url : urls) {
		auto path = url.toString();
		if (path.startsWith(LOCAL_SNAPSHOT_PREFIX)) {
			const int sourceIndex = path.remove(0, LOCAL_SNAPSHOT_PREFIX.length()).toInt();
			if (sourceIndex < (int)m_manager.count()) {
				m_manager.move(m_manager.get(sourceIndex), newIndex <= getCurrentIndex() ? newIndex : newIndex - 1);
			}
		}
	}
}

// ------------------------------------------------------------------------ //

SnapshotProxy::SnapshotProxy(ISnapshot & snapshot)
	: m_snapshot(snapshot)
	, m_name("Name")
	, m_hasCameraState("Camera state")
	, m_hasTransformations("Transformations")
	, m_hasVisibility("Visibility")
	, m_hasAssignedMaterials("Assigned materials")
	, m_hasEnvironment("Environment")
	, m_hasBackground("Background") {
	m_name.setParam(&snapshot.name());
	m_hasCameraState.setParam(&snapshot.hasCameraState());
	m_hasTransformations.setParam(&snapshot.hasTransformations());
	m_hasVisibility.setParam(&snapshot.hasVisibility());
	m_hasAssignedMaterials.setParam(&snapshot.hasAssignedMaterials());
	m_hasEnvironment.setParam(&snapshot.hasEnvironment());
	m_hasBackground.setParam(&snapshot.hasBackground());

	m_data["name"] = QVariant::fromValue(static_cast<QObject *>(&m_name));
	m_data["hasCameraState"] = QVariant::fromValue(static_cast<QObject *>(&m_hasCameraState));
	m_data["hasTransformations"] = QVariant::fromValue(static_cast<QObject *>(&m_hasTransformations));
	m_data["hasVisibility"] = QVariant::fromValue(static_cast<QObject *>(&m_hasVisibility));
	m_data["hasAssignedMaterials"] = QVariant::fromValue(static_cast<QObject *>(&m_hasAssignedMaterials));
	m_data["hasEnvironment"] = QVariant::fromValue(static_cast<QObject *>(&m_hasEnvironment));
	m_data["hasBackground"] = QVariant::fromValue(static_cast<QObject *>(&m_hasBackground));

	updatePreview();
}

void SnapshotProxy::updatePreview() {
	auto image = m_snapshot.getPreview();
	if (!image.isNull()) {
		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		buffer.open(QIODevice::WriteOnly);
		image.save(&buffer, "PNG");
		m_data["preview"] = "data:image/png;base64," + byteArray.toBase64();
	}
}
