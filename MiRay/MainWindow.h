#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>

#include "../Shared/Interfaces/Scene.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

	static MainWindow * m_instance;
	IApplicationContext	& m_appCtx;
	QSettings m_settings;

	QScopedPointer<class MainWidget> m_mainWidget;

	void closeEvent(QCloseEvent * event) override;

public:
	explicit MainWindow(IApplicationContext	& ctx, QWidget *parent = nullptr);
	~MainWindow() override;

	static MainWindow * instance() { return m_instance; }
	static IApplicationContext & appContext() { return m_instance->m_appCtx; }
	QSettings & settings() { return m_instance->m_settings; }

	void openFile(const QString & path, bool import = false);
	void updateTitle(const QString & fileName, int tabCount);

private slots:
	void delayedInitWindow();
};

#endif // MAINWINDOW_H
