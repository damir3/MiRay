#pragma once

#include "ParamProxy.h"

#include <QStandardItem>
#include <QQuickItem>

class ISnapshot;
class ISnapshotManager;

class SnapshotProxy : public QObject {
	Q_OBJECT

	ISnapshot &			m_snapshot;

	StringParamProxy	m_name;
	BooleanParamProxy	m_hasCameraState;
	BooleanParamProxy	m_hasTransformations;
	BooleanParamProxy	m_hasVisibility;
	BooleanParamProxy	m_hasAssignedMaterials;
	BooleanParamProxy	m_hasEnvironment;
	BooleanParamProxy	m_hasBackground;

	QVariantMap 		m_data;

public:
	SnapshotProxy(ISnapshot & snapshot);

	void updatePreview();
	const QVariantMap & modelData() const { return m_data; }
};

class SnapshotInfo : public QObject {
	Q_OBJECT

	ISnapshotManager & m_manager;
	ISnapshot * m_snapshot;
	int m_currentIndex;
	QQmlContext * m_qmlContext;
	QQuickItem * m_quickItem;
	std::vector<std::unique_ptr<SnapshotProxy>> m_proxyObjects;
	QScopedPointer<QStandardItemModel> m_snapshotsList;

	int getCurrentIndex() const;
	void updateCurrentIndex();

public:
	SnapshotInfo(ISnapshotManager & manager);
	~SnapshotInfo();

	void setQmlContext(QQmlContext * qmlContext, QQuickItem * quickItem);
	void setSnapshot(ISnapshot * snapshot);

	Q_INVOKABLE void setSnapshot(int index);
	Q_INVOKABLE void createSnapshot();
	Q_INVOKABLE void deleteSnapshot();
	Q_INVOKABLE void updateSnapshot();
	Q_INVOKABLE void activateSnapshot();
	Q_INVOKABLE bool canDropUrls(QList<QUrl> urls, int index);
	Q_INVOKABLE void dropUrls(QList<QUrl> urls, int index);

public slots:
	void onSnapshotListChanged();
};
