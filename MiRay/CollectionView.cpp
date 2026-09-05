#include "CollectionView.h"

#include <QDir>
#include <QDirIterator>
#include <QStandardItem>
#include <QTimer>
#include <QFile>

#include "../Shared/Interfaces/Image.h"
#include "../Shared/Utils/FileUtils.h"

enum NodeRoles {
	NameRole = Qt::UserRole + 1,
	PathRole
};

static QStandardItem * createTreeItem(const QString & path, QStandardItem * parent)
{
	auto item = new QStandardItem();
	item->setData(parent ? QFileInfo(path).fileName() : "All", NameRole);
	item->setData(path + "/", PathRole);

	QDir dir(path);
	for (const auto & subDir : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
		item->appendRow(createTreeItem(path + "/" + subDir, item));
	}

	return item;
}

LibraryCollection::LibraryCollection(const QString & path, const QStringList & exts)
	: m_dataModel(new ListModel(QDir::fromNativeSeparators(path), exts))
	, m_filterModel(new FilterProxyModel(QDir::fromNativeSeparators(path), this))
	, m_treeModel(new QStandardItemModel())
{
	const auto normPath = QDir::fromNativeSeparators(path);
	QHash<int, QByteArray> roleNames;
	roleNames[NameRole] = "name";
	roleNames[PathRole] = "path";
	m_treeModel->setItemRoleNames(roleNames);

	m_treeModel->appendRow(createTreeItem(normPath, nullptr));

	m_filterModel->setSourceModel(m_dataModel.data());
}

static QByteArray readThumbnailFile(const QString &fileName)
{
	QFile file(fileName);
	return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

static QString getMaterialThumbnail(const QString & url)
{
	auto data = readThumbnailFile(url);
	if (data.startsWith('{')) {
		auto index = data.indexOf("\"thumbnail\"");
		if (index >= 0) {
			index += 11;
			auto startQuote = data.indexOf('"', index);
			if (startQuote >= 0) {
				auto endQuote = data.indexOf('"', startQuote + 1);
				if (endQuote > startQuote) {
					return "data:image/png;base64," + QString(data.mid(startQuote + 1, endQuote - startQuote - 1));
				}
			}
		}
		// QJsonParseError parseError;
		// QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
		// if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull() && jsonDoc.isObject()) {
		// 	QJsonObject mtlObj = jsonDoc.object();
		// 	if (mtlObj.contains("thumbnail")) {
		// 		QString thumb = mtlObj["thumbnail"].toString();
		// 		if (!thumb.isEmpty()) {
		// 			return "data:image/png;base64," + thumb;
		// 		}
		// 	}
		// }
		return QString();
	}

	auto begin = data.indexOf("<thumbnail>");
	if (begin >= 0) {
		data.remove(0, begin + 11);
		auto end = data.indexOf("</thumbnail>");
		if (end > 0) {
			data.resize(end);
			return "data:image/png;base64," + QString(data);
		}
	}
	return QString();
}

ListModel::ListModel(const QString & path, const QStringList & exts) : m_path(path), m_extensions(exts)
{
	QTimer::singleShot(10, this, &ListModel::delayedInit);
}

void ListModel::delayedInit()
{
	qDebug() << "Load library collection" << m_path;
	beginResetModel();

	QStringList urls;
	QDirIterator it(m_path, m_extensions, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) urls.push_back(it.next());

	urls.sort();
	for (const auto & url : urls) {
		QStringList item;
		QFileInfo fileInfo(url);
		auto localPath = url;
		localPath.remove(0, m_path.length() + 1);
		localPath = localPath.left(localPath.lastIndexOf('.'));
		item.push_back(fileInfo.completeBaseName());

		QString thumbnail;
		if (QFile::exists(url + ".thumbnail")) {
			thumbnail = "data:image/png;base64," + readFile(url + ".thumbnail").toBase64();
		} else if (QFile::exists(url + ".jpg")) {
			thumbnail = "data:image/jpg;base64," + readFile(url + ".jpg").toBase64();
		} else if (QFile::exists(url + ".png")) {
			thumbnail = "data:image/png;base64," + readFile(url + ".png").toBase64();
		} else if (!fileInfo.suffix().compare("mirayMaterial", Qt::CaseInsensitive)) {
			thumbnail = getMaterialThumbnail(url);
		} else {
			auto ext = fileInfo.suffix();
			if (!fileInfo.suffix().compare("jpg", Qt::CaseInsensitive) ||
				!fileInfo.suffix().compare("png", Qt::CaseInsensitive) ||
				!fileInfo.suffix().compare("exr", Qt::CaseInsensitive) ||
				!fileInfo.suffix().compare("hdr", Qt::CaseInsensitive)) {
				auto imageManager = qApp->imageManager();
				if (auto image = imageManager->loadImage(url, false, eImageColorSpace::sRGB)) {
					if (auto thumbnailImage = imageManager->scaleImage(image, 160, 160, eImageFormat::RGBA, eImageDataType::Byte, eScaleFilter::Triangle)) {
						auto pngData = imageManager->saveImage(thumbnailImage, "png", 1.0f);
						if (!pngData.isEmpty()) {
							writeFile(url + ".thumbnail", pngData);
							thumbnail = "data:image/png;base64," + pngData.toBase64();
						}
					}
				}
			}
		}

		item.push_back(thumbnail);
		item.push_back(url);
		item.push_back(localPath);
		item.push_back(qApp->getShortResourcePath(url));
		m_items.push_back(item);
	}

	endResetModel();
}

int ListModel::rowCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);
	return m_items.count();
}

QVariant ListModel::data(const QModelIndex & index, int role) const
{
	if (index.row() < 0 || index.row() >= m_items.count() || role < 0 || role >= m_items[index.row()].count())
		return QVariant();

	return m_items[index.row()][role];
}

QHash<int, QByteArray> ListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[0] = "name";
	roles[1] = "thumbnail";
	roles[2] = "path";
	roles[3] = "localPath";
	roles[4] = "shortPath";
	return roles;
}

FilterProxyModel::FilterProxyModel(const QString & path, QObject *parent)
	: QSortFilterProxyModel(parent)
	, m_folder(path)
{
	setFilterRole(0);
	setFilterCaseSensitivity(Qt::CaseInsensitive);
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const
{
	const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
	return sourceModel()->data(index, 2).toString().startsWith(m_folder) &&
		sourceModel()->data(index, 3).toString().contains(filterRegExp()); // local path
}

void FilterProxyModel::setFolder(QString path)
{
	m_folder = QDir::fromNativeSeparators(path);
	invalidate();
}

void FilterProxyModel::setFilter(QString string)
{
	setFilterWildcard(string);
}
