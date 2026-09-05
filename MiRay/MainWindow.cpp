#include "MainWindow.h"
#include "MainWidget.h"

#include <QTimer>
#include <QCloseEvent>
#include <QFileInfo>

MainWindow * MainWindow::m_instance = nullptr;

MainWindow::MainWindow(IApplicationContext & ctx, QWidget * parent)
	: QMainWindow(parent)
	, m_appCtx(ctx)
	, m_settings("TuiSoftware", "MiRay")
{
	m_instance = this;

	setWindowTitle("MiRay");
	setMinimumSize(1140, 600);

	restoreGeometry(m_settings.value("window/geometry").toByteArray());
	restoreState(m_settings.value("window/state").toByteArray());

	QTimer::singleShot(10, this, SLOT(delayedInitWindow()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::delayedInitWindow()
{
	m_mainWidget.reset(new MainWidget(*this, m_appCtx));
	setCentralWidget(m_mainWidget.data());

	setAcceptDrops(true);
	updateTitle("", 0);
}

void MainWindow::openFile(const QString & path, bool import)
{
	if (m_mainWidget)
		m_mainWidget->openFile(path, import);
}

void MainWindow::updateTitle(const QString & fileName, int tabCount)
{
	QString title = "MiRay";
	if (!fileName.isEmpty())
		title = QFileInfo(fileName).fileName() + " - " + title;
	setWindowTitle(title);
}

void MainWindow::closeEvent(QCloseEvent * event)
{
	if (m_mainWidget && !m_mainWidget->exit()) {
		event->ignore();
		return;
	}

	event->accept();
	m_settings.setValue("window/geometry", saveGeometry());
	m_settings.setValue("window/state", saveState());

	QMainWindow::closeEvent(event);
}
