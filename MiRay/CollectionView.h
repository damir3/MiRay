#pragma once

#include <QStandardItemModel>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class ListModel : public QAbstractListModel
{
	Q_OBJECT

	const QString m_path;
	const QStringList m_extensions;
	QVector<QStringList> m_items;

	void delayedInit();

public:
	ListModel(const QString & path, const QStringList & exts);

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
};

class FilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	QString	m_folder;

public:
	FilterProxyModel(const QString & path, QObject * parent);

	bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const override;

	Q_INVOKABLE void setFolder(QString string);
	Q_INVOKABLE void setFilter(QString string);
};

class LibraryCollection : public QObject
{
	Q_OBJECT

	QScopedPointer<QAbstractListModel>    m_dataModel;
	QScopedPointer<QSortFilterProxyModel> m_filterModel;
	QScopedPointer<QStandardItemModel>    m_treeModel;

public:
	LibraryCollection(const QString & path, const QStringList & exts);

	QSortFilterProxyModel * filterModel() const { return m_filterModel.data(); }
	QStandardItemModel * treeModel() const { return m_treeModel.data(); }
};
