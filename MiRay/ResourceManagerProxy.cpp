#include "ResourceManagerProxy.h"

#include <QFileDialog>

#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Image.h"
#include "../Shared/Utils/FileUtils.h"

ResourceManagerProxy::ResourceManagerProxy(QWidget * widget, IScene & scene)
	: m_widget(widget)
	, m_scene(scene)
	, m_currentIndex(-1)
	, m_undoPosition(0)
{
	QSet<QString> res;
	m_scene.collectFileNames(res);
	for (auto & path : res)
		m_images.emplace_back(std::make_unique<ResourceImage>(path, *this));

	std::sort(m_images.begin(), m_images.end(), [](const std::unique_ptr<ResourceImage> & a, const std::unique_ptr<ResourceImage> & b) {
		return a->oldPath().compare(b->oldPath(), Qt::CaseInsensitive) < 0;
	});

	for (auto & image : m_images)
		m_imagesList.push_back(QVariant::fromValue(static_cast<QObject *>(image.get())));
}

ResourceManagerProxy::~ResourceManagerProxy()
{
}

QString ResourceManagerProxy::getPreview(const QString & path)
{
	auto it = m_previews.find(path);
	if (it != m_previews.end())
		return it->second;

	auto * imageManager = qApp->imageManager();
	auto preview = imageManager->getPreviewBase64(path, 256, 0);
	m_previews[path] = preview;
	return preview;
}

void ResourceManagerProxy::select(int index, int modifiers)
{
	if (modifiers & Qt::ControlModifier)
	{
		if (m_selection.find(index) != m_selection.end())
			m_selection.erase(index);
		else
			m_selection.insert(index);
	}
	else if (modifiers & Qt::ShiftModifier)
	{
		if (index >= 0 && index < (int)m_images.size())
			m_selection.insert(index);
	}
	else
	{
		m_selection.clear();
		if (index >= 0 && index < (int)m_images.size())
			m_selection.insert(index);
	}

	for (int i = 0; i < (int)m_images.size(); i++)
		m_images[i]->setSelected(m_selection.find(i) != m_selection.end());

	m_currentIndex = index;

	emit selectionChanged();
}

void ResourceManagerProxy::changeFolder()
{
	if (m_lastDir.isEmpty() && m_selection.size() > 0)
	{
		auto * image = m_images[*m_selection.begin()].get();
		m_lastDir = QFileInfo(image->newPath()).absolutePath();
	}

	UndoCommand cmd;

	const auto newDir = QFileDialog::getExistingDirectory(m_widget, "Select Folder", m_lastDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!newDir.isEmpty())
	{
		m_lastDir = newDir;

		for (auto index : m_selection)
		{
			auto * image = m_images[index].get();
			const auto fileName = QFileInfo(image->oldPath()).fileName();
			auto newPath = nativePath(QString("%1/%2").arg(newDir).arg(fileName));
			cmd.changes.emplace_back(Change{ image, newPath });
		}
	}

	if (!cmd.changes.empty())
	{
		assert(m_undoPosition <= m_undo.size());
		m_undo.resize(m_undoPosition);
		m_undo.emplace_back(cmd);
		redo();
	}
}

void ResourceManagerProxy::changeFile()
{
	if (m_selection.size() != 1)
		return;

	auto * image = m_images[*m_selection.begin()].get();

	const auto currentFile = qApp->getFullResourcePath(image->newPath());
	const auto currentSuffix = QFileInfo(currentFile).suffix();

	UndoCommand cmd;

	const auto newPath = getOpenFileName(m_widget, "Select File", currentSuffix, currentFile, qApp->imageManager()->loadingFilters());
	if (!newPath.isEmpty())
		cmd.changes.emplace_back(Change{ image, newPath });

	if (!cmd.changes.empty()) {
		assert(m_undoPosition <= m_undo.size());
		m_undo.resize(m_undoPosition);
		m_undo.emplace_back(cmd);
		redo();
	}
}

void ResourceManagerProxy::reset()
{
	UndoCommand cmd;

	for (auto index : m_selection) {
		auto image = m_images[index].get();
		cmd.changes.emplace_back(Change{ image, image->oldPath() });
	}

	if (!cmd.changes.empty()) {
		assert(m_undoPosition <= m_undo.size());
		m_undo.resize(m_undoPosition);
		m_undo.emplace_back(cmd);
		redo();
	}
}

void ResourceManagerProxy::clear()
{
	UndoCommand cmd;

	for (auto index : m_selection)
		cmd.changes.emplace_back(Change{ m_images[index].get(), QString() });

	if (!cmd.changes.empty())
	{
		assert(m_undoPosition <= m_undo.size());
		m_undo.resize(m_undoPosition);
		m_undo.emplace_back(cmd);
		redo();
	}
}

ResourceManagerProxy::Change::Change(ResourceImage * target, const QString & path) : image(target), oldPath(target->newPath()), newPath(path) {}

void ResourceManagerProxy::UndoCommand::undo()
{
	for (auto & change : changes)
		change.image->setNewPath(change.oldPath);
}

void ResourceManagerProxy::UndoCommand::redo()
{
	for (auto & change : changes)
		change.image->setNewPath(change.newPath);
}

bool ResourceManagerProxy::canUndo() const
{
	return m_undoPosition > 0 && m_undoPosition <= m_undo.size();
}

void ResourceManagerProxy::undo()
{
	if (canUndo())
	{
		m_undo[--m_undoPosition].undo();
		emit undoChanged();
	}
}

bool ResourceManagerProxy::canRedo() const
{
	return m_undoPosition < m_undo.size();
}

void ResourceManagerProxy::redo()
{
	if (canRedo())
	{
		m_undo[m_undoPosition++].redo();
		emit undoChanged();
	}
}

void ResourceManagerProxy::accept()
{
	QMap<QString, QString> resourceMap;
	for (auto & image : m_images)
	{
		if (image->newPath() != image->oldPath())
			resourceMap[image->oldPath()] = image->newPath();
	}

	if (!resourceMap.isEmpty())
		m_scene.updateFileNames(resourceMap);
}

// ------------------------------------------------------------------------ //

ResourceImage::ResourceImage(const QString & path, ResourceManagerProxy & owner)
	: m_owner(owner)
	, m_exists(QFileInfo(qApp->getFullResourcePath(path)).exists())
	, m_oldPath(path)
	, m_newPath(path)
	, m_selected(false)
{
}

void ResourceImage::setNewPath(const QString & path)
{
	m_newPath = path;
	m_exists = QFileInfo(qApp->getFullResourcePath(path)).exists();
	emit changed();
}

void ResourceImage::setSelected(bool b)
{
	if (m_selected != b)
	{
		m_selected = b;
		emit selectionChanged();
	}
}
