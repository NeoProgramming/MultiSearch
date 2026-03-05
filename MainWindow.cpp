#include "MainWindow.h"
#include <QTabWidget>
#include <QDockWidget>
#include <QWebEngineView>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QFileDialog>
#include "FileExtractor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
	, m_tabWidget(nullptr)
	, m_searchDockWidget(nullptr)
	, m_searchDock(nullptr)
{
	setupUi();

	// Настройки окна
	setWindowTitle("MultiSearch");
	resize(1200, 800);

}

MainWindow::~MainWindow()
{}

void MainWindow::setupUi()
{
	// Создаем меню
	QMenuBar* menuBar = new QMenuBar(this);
	QMenu* fileMenu = menuBar->addMenu("File");
	QMenu* viewMenu = menuBar->addMenu("View");
	QMenu* helpMenu = menuBar->addMenu("Help");

	// Действия меню Файл
	QAction* openAsTextAction = new QAction("&Open as text...", this);
	openAsTextAction->setShortcut(QKeySequence::Open);
	connect(openAsTextAction, &QAction::triggered, this, &MainWindow::onFileOpenAsText);
	fileMenu->addAction(openAsTextAction);

	fileMenu->addSeparator();

	QAction* exitAction = new QAction("Exit", this);
	exitAction->setShortcut(QKeySequence::Quit);
	connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
	fileMenu->addAction(exitAction);



	// Действия меню Вид
	QAction* toggleSearchDockAction = new QAction("Search", this);
	toggleSearchDockAction->setCheckable(true);
	toggleSearchDockAction->setChecked(true);
	// connect позже, когда создадим док-панель

	viewMenu->addAction(toggleSearchDockAction);

	setMenuBar(menuBar);

	// Создаем строку статуса
	QStatusBar* statusBar = new QStatusBar(this);
	statusBar->showMessage("Ready");
	setStatusBar(statusBar);

	// Создаем док-панель поиска
	createSearchDock();

	// Связываем действие с док-панелью
	connect(toggleSearchDockAction, &QAction::toggled,
		m_searchDockWidget, &QDockWidget::setVisible);

	// Создаем область с табами
	createTabWidget();
}

void MainWindow::createSearchDock()
{
	// Создаем док-панель
	m_searchDockWidget = new QDockWidget("Search", this);
	m_searchDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	// Создаем содержимое док-панели
	m_searchDock = new SearchDock(m_searchDockWidget);
	m_searchDockWidget->setWidget(m_searchDock);

	// Добавляем док-панель в левую область
	addDockWidget(Qt::LeftDockWidgetArea, m_searchDockWidget);

	// Подключаем сигнал поиска
	connect(m_searchDock, &SearchDock::searchRequested,
		this, &MainWindow::onSearchRequested);
	connect(m_searchDock, &SearchDock::fileDoubleClicked,
		this, &MainWindow::onFileDoubleClicked);
}

void MainWindow::createTabWidget()
{
	// Создаем виджет с табами
	m_tabWidget = new QTabWidget(this);
	m_tabWidget->setTabsClosable(true);
	m_tabWidget->setMovable(true);

	// Подключаем сигналы
	connect(m_tabWidget, &QTabWidget::tabCloseRequested,
		this, &MainWindow::onTabCloseRequested);
	connect(m_tabWidget, &QTabWidget::currentChanged,
		this, &MainWindow::onCurrentTabChanged);

	// Создаем приветственный таб
	QWebEngineView* welcomeView = new QWebEngineView();
	welcomeView->setHtml(
		"<html><body style='font-family: sans-serif; padding: 20px;'>"
		"<h1>Welcome!</h1>"
		"<p>Use the search bar on the left to search for text in files.</p>"
		"<p>Supported formats: txt, html, htm, mht, mhtml</p>"
		"</body></html>"
	);

	int index = m_tabWidget->addTab(welcomeView, "Main");
	m_tabWidget->setCurrentIndex(index);

	setCentralWidget(m_tabWidget);
}

void MainWindow::onTabCloseRequested(int index)
{
	// Не закрываем последнюю вкладку
	if (m_tabWidget->count() > 1) {
		QWidget* tab = m_tabWidget->widget(index);
		m_tabWidget->removeTab(index);
		delete tab;
	}
}

void MainWindow::onCurrentTabChanged(int index)
{
	if (index >= 0) {
		QWebEngineView* view = qobject_cast<QWebEngineView*>(m_tabWidget->widget(index));
		if (view) {
			statusBar()->showMessage("View: " + m_tabWidget->tabText(index));
		}
	}
}

void MainWindow::onSearchRequested()
{
	QString path = m_searchDock->getSearchPath();
	QStringList words = m_searchDock->getSearchWords();

	if (path.isEmpty()) {
		statusBar()->showMessage("Please specify file or folder path", 3000);
		return;
	}

	if (words.isEmpty()) {
		statusBar()->showMessage("Please enter words to search", 3000);
		return;
	}

	statusBar()->showMessage(QString("Searching for '%1' in %2...")
		.arg(words.join(" "))
		.arg(path));

	// Очищаем предыдущие результаты
	m_searchDock->clearResults();

	// TODO: здесь будет реальный поиск
	// Пока добавим тестовые данные
	m_searchDock->addResult("test1.html", "C:\\test1.html", 3);
	m_searchDock->addResult("test2.html", "C:\\test2.html", 5);
}

void MainWindow::onFileDoubleClicked(const QString& filePath)
{
	statusBar()->showMessage(QString("Opening: %1").arg(filePath), 3000);

	// TODO: здесь будет открытие файла в новой вкладке
	// Создаем новый WebEngineView
	QWebEngineView* view = new QWebEngineView();
	view->setUrl(QUrl::fromLocalFile(filePath));

	// Добавляем вкладку
	QFileInfo fileInfo(filePath);
	int index = m_tabWidget->addTab(view, fileInfo.fileName());
	m_tabWidget->setCurrentIndex(index);
}

void MainWindow::onFileOpenAsText()
{
	QString filePath = QFileDialog::getOpenFileName(
		this,
		"Open file as text",
		QString(),
		"All Files (*.*);;HTML Files (*.html *.htm);;Text Files (*.txt);;MHTML Files (*.mht *.mhtml)"
	);

	if (filePath.isEmpty())
		return;

	statusBar()->showMessage(QString("Loading: %1").arg(filePath));

	// Загружаем и декодируем файл
	QString decodedText = FileExtractor::loadFile(filePath);

	if (decodedText.isEmpty()) {
		statusBar()->showMessage("Failed to decode file or file is empty", 3000);
		return;
	}

	// Создаем информативный заголовок для таба
	QFileInfo fileInfo(filePath);
	QString tabTitle = QString("[TEXT] %1").arg(fileInfo.fileName());

	// Добавляем отладочную информацию о размере
	QString debugInfo = QString(
		"File: %1\n"
		"Size: %2 bytes\n"
		"Decoded text length: %3 characters\n"
		"First 500 characters:\n\n"
	).arg(filePath)
		.arg(fileInfo.size())
		.arg(decodedText.length());

	QString displayText = debugInfo + decodedText;
	// Показываем первые 500 символов для проверки
//	QString displayText = debugInfo + decodedText.left(500);
//	if (decodedText.length() > 500) {
//		displayText += "\n\n...[truncated]...";
//	}

	createTextTab(tabTitle, displayText);

	statusBar()->showMessage(QString("Loaded: %1 (%2 chars)")
		.arg(fileInfo.fileName())
		.arg(decodedText.length()), 3000);
}

void MainWindow::createTextTab(const QString& title, const QString& text)
{
	// Создаем простой HTML для отображения текста
	QString html = QString(
		"<!DOCTYPE html>"
		"<html>"
		"<head>"
		"<style>"
		"body { font-family: 'Courier New', monospace; white-space: pre-wrap; padding: 20px; }"
		".debug-info { background-color: #f0f0f0; padding: 10px; border-left: 4px solid #0078d7; margin-bottom: 20px; }"
		"</style>"
		"</head>"
		"<body>"
		"<div class='debug-info'>%1</div>"
		"<div>%2</div>"
		"</body>"
		"</html>"
	).arg(text.toHtmlEscaped().left(500).replace("\n", "<br>")) // Просто для примера, но лучше передавать чистый текст
		.arg(text.toHtmlEscaped().replace("\n", "<br>"));

	// Но чтобы не дублировать, лучше так:
	html = QString(
		"<!DOCTYPE html>"
		"<html>"
		"<head>"
		"<style>"
		"body { font-family: 'Courier New', monospace; white-space: pre-wrap; padding: 20px; }"
		"</style>"
		"</head>"
		"<body>"
		"%1"
		"</body>"
		"</html>"
	).arg(text.toHtmlEscaped().replace("\n", "<br>"));

	QWebEngineView* view = new QWebEngineView();
	view->setHtml(html);

	int index = m_tabWidget->addTab(view, title);
	m_tabWidget->setCurrentIndex(index);
}
