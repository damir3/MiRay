#pragma once

class IScene;

class ResourceManagerProxy : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList images MEMBER m_imagesList CONSTANT)
	Q_PROPERTY(int selectionCount READ getSelectionCount NOTIFY selectionChanged)
	Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoChanged)
	Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoChanged)

	QWidget * 		m_widget;
	IScene & 		m_scene;
	QVariantList	m_imagesList;
	std::vector<std::unique_ptr<class ResourceImage>>	m_images;
	std::map<QString, QString> m_previews;
	std::set<int>	m_selection;
	QString			m_lastDir;
	int				m_currentIndex;

	struct Change {
		ResourceImage * image;
		QString oldPath;
		QString newPath;

		Change(ResourceImage * target, const QString & path);
	};
	struct UndoCommand {
		std::vector<Change> changes;

		void undo();
		void redo();
	};
	std::vector<UndoCommand> m_undo;
	size_t m_undoPosition;

	int getSelectionCount() const { return (int)m_selection.size(); }
	bool canUndo() const;
	bool canRedo() const;

public:
	ResourceManagerProxy(QWidget * widget, IScene & scene);
	~ResourceManagerProxy();

	QString getPreview(const QString & path);

	Q_INVOKABLE void select(int index, int modifiers);
	Q_INVOKABLE void changeFolder();
	Q_INVOKABLE void changeFile();
	Q_INVOKABLE void reset();
	Q_INVOKABLE void clear();
	Q_INVOKABLE void undo();
	Q_INVOKABLE void redo();
	Q_INVOKABLE void accept();

signals:
	void selectionChanged();
	void undoChanged();
};

class ResourceImage : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool status MEMBER m_exists NOTIFY changed)
	Q_PROPERTY(QString oldPath MEMBER m_oldPath CONSTANT)
	Q_PROPERTY(QString newPath MEMBER m_newPath NOTIFY changed)
	Q_PROPERTY(QString preview READ getPreview NOTIFY changed)
	Q_PROPERTY(bool selected MEMBER m_selected NOTIFY selectionChanged)

	ResourceManagerProxy & m_owner;
	const QString	m_oldPath;
	QString			m_newPath;
	bool			m_exists;
	bool			m_selected;

public:
	ResourceImage(const QString & path, ResourceManagerProxy & owner);

	const QString & oldPath() const { return m_oldPath; }
	const QString & newPath() const { return m_newPath; }
	void setNewPath(const QString & path);
	QString getPreview() const { return m_owner.getPreview(m_newPath); }
	void setSelected(bool b);

signals:
	void changed();
	void selectionChanged();
};
