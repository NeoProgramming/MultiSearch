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
#include <QApplication>
#include <QDirIterator>

#include "FileExtractor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
	, m_tabWidget(nullptr)
	, m_searchDockWidget(nullptr)
	, m_searchDock(nullptr)
{
	// Загружаем настройки
	cfg.loadSettings();

	setupUi();

	// Настройки окна
	setWindowTitle("MultiSearch");
	resize(1200, 800);

}

MainWindow::~MainWindow()
{
	cfg.saveSettings();
}

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

	if (words.size() < 2) {
		statusBar()->showMessage("Please enter at least two words", 3000);
		return;
	}

	// Берем только первые два слова для начала
	QString word1 = words[0];
	QString word2 = words[1];

	statusBar()->showMessage(QString("Searching for '%1' and '%2' in %3...")
		.arg(word1, word2, path), 0);

	// Очищаем предыдущие результаты
	m_searchDock->clearResults();

	// Настройки поиска
	SearchEngine::Config config;
	config.caseSensitive = m_searchDock->isCaseSensitive();
	config.radius = m_searchDock->getSearchRadius();
	SearchEngine searcher(config);

	QFileInfo pathInfo(path);
	int totalMatches = 0;

	if (pathInfo.isFile()) {
		// Поиск в одном файле
		processFile(path, word1, word2, searcher, totalMatches);
	}
	else if (pathInfo.isDir()) {
		// Поиск в директории рекурсивно
		QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
		int fileCount = 0;

		while (it.hasNext()) {
			QString filePath = it.next();
			// Фильтруем только текстовые файлы
			if (filePath.endsWith(".txt", Qt::CaseInsensitive)) {
				processFile(filePath, word1, word2, searcher, totalMatches);

				fileCount++;
				if (fileCount % 10 == 0) {
					statusBar()->showMessage(QString("Processed %1 files...")
						.arg(fileCount));
					QApplication::processEvents();
				}
			}
		}
	}

	statusBar()->showMessage(QString("Search completed. Found %1 matches in %2 files.")
		.arg(totalMatches)
		.arg(m_searchDock->getResultCount()), 5000);
}

void MainWindow::processFile(const QString& filePath,
	const QString& word1,
	const QString& word2,
	const SearchEngine& searcher,
	int& totalMatches)
{
	qDebug() << "Processing file:" << filePath;

	QString text = FileExtractor::loadFile(filePath, FileExtractor::ExtractTextOnly);
	if (text.isEmpty()) {
		qDebug() << "File is empty or could not be loaded";
		return;
	}

	qDebug() << "Text length:" << text.length();

	auto matches = searcher.findTwoWords(text, word1, word2);

	qDebug() << "Found matches:" << matches.size();

	if (!matches.isEmpty()) {
		QFileInfo fileInfo(filePath);
		QString context = matches.first().surroundingText;
		m_searchDock->addResult(fileInfo.fileName(), filePath,
			matches.size(), context);
		totalMatches += matches.size();

		// Сохраняем все совпадения для этого файла
		m_fileMatches[filePath] = matches;
		qDebug() << "Saved matches for" << filePath << "count:" << matches.size();
	}
}

void MainWindow::onFileDoubleClicked(const QString& filePath)
{
	if (!m_fileMatches.contains(filePath)) {
		statusBar()->showMessage("No match data for this file", 3000);
		return;
	}

	const auto& matches = m_fileMatches[filePath];
	if (matches.isEmpty()) return;

	// Открываем файл и подсвечиваем первое совпадение
	openFileWithHighlights(filePath, matches);
}

void MainWindow::openFileWithHighlights(const QString& filePath,
	const QVector<SearchMatch>& matches)
{
	QWebEngineView* view = new QWebEngineView();

	QString text = FileExtractor::loadFile(filePath, FileExtractor::ExtractFull);

	if (text.isEmpty()) {
		view->setHtml("<html><body>Error loading file</body></html>");
		int index = m_tabWidget->addTab(view, QFileInfo(filePath).fileName());
		m_tabWidget->setCurrentIndex(index);
		return;
	}

	// Генерируем HTML
	QString html = generateHighlightedHtml(text, matches);

	// Создаем временный файл в системной temp директории
	QString tempDir = QDir::temp().absoluteFilePath("multisearch");
	QDir().mkpath(tempDir);

	QString tempFilePath = tempDir + "/" +
		QFileInfo(filePath).fileName() + "_" +
		QString::number(QDateTime::currentMSecsSinceEpoch()) +
		".html";

	QFile tempFile(tempFilePath);
	if (tempFile.open(QIODevice::WriteOnly)) {
		tempFile.write(html.toUtf8());
		tempFile.close();
		qDebug() << "Saved highlighted file to:" << tempFilePath;

		view->setUrl(QUrl::fromLocalFile(tempFilePath));

		// Опционально: удалить временный файл при закрытии вкладки
		connect(view, &QObject::destroyed, [tempFilePath]() {
			QFile::remove(tempFilePath);
		});
	}
	else {
		// Fallback
		view->setHtml(html.left(1000000)); // Только начало файла
	}

	int index = m_tabWidget->addTab(view, QFileInfo(filePath).fileName());
	m_tabWidget->setCurrentIndex(index);
}

QString MainWindow::generateHighlightedHtml(const QString& text,
	const QVector<SearchMatch>& matches)
{
	QString html = "<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"    <meta charset='utf-8'>\n"
		"    <style>\n"
		"        body { \n"
		"            font-family: 'Segoe UI', Arial, sans-serif; \n"
		"            line-height: 1.6;\n"
		"            padding: 20px; \n"
		"            max-width: 900px;\n"
		"            margin: 0 auto;\n"
		"            background-color: white;\n"
		"            color: #333;\n"
		"        }\n"
		"        p { \n"
		"            margin: 0 0 1em 0;\n"
		"            text-align: justify;\n"
		"        }\n"
		"        .empty-line {\n"
		"            height: 1em;\n"
		"        }\n"
		"        .match { \n"
		"            background-color: #ffff00; \n"
		"            font-weight: 500;\n"
		"            padding: 2px 0;\n"
		"            border-radius: 3px;\n"
		"        }\n"
		"        .match-info { \n"
		"            position: sticky; \n"
		"            top: 0; \n"
		"            background: #f8f9fa; \n"
		"            padding: 10px 20px; \n"
		"            border-bottom: 2px solid #0078d7;\n"
		"            margin-bottom: 20px;\n"
		"            z-index: 1000;\n"
		"            font-size: 14px;\n"
		"            box-shadow: 0 2px 5px rgba(0,0,0,0.1);\n"
		"        }\n"
		"        .match-counter {\n"
		"            font-weight: bold;\n"
		"            color: #0078d7;\n"
		"        }\n"
		"    </style>\n"
		"</head>\n"
		"<body>\n";

	// Информационная панель
	html += QString("<div class='match-info'>"
		"Найдено <span class='match-counter'>%1</span> %2"
		"</div>\n")
		.arg(matches.size())
		.arg(matches.size() == 1 ? "совпадение" :
		(matches.size() < 5 ? "совпадения" : "совпадений"));

	// Разбиваем на строки, сохраняя пустые
	QStringList lines = text.split('\n');

	int currentPos = 0;
	for (const QString& line : lines) {
		if (line.isEmpty()) {
			// Пустая строка - вставляем разделитель
			html += "<div class='empty-line'></div>\n";
			currentPos++;
			continue;
		}

		html += "<p>";

		int lineStart = currentPos;
		int lineEnd = lineStart + line.length();

		// Находим все совпадения в этой строке
		int lastPos = lineStart;
		for (const auto& match : matches) {
			if (match.startPos >= lineEnd) break;

			if (match.endPos > lineStart && match.startPos < lineEnd) {
				// Текст до совпадения
				if (match.startPos > lastPos) {
					html += line.mid(lastPos - lineStart,
						match.startPos - lastPos).toHtmlEscaped();
				}

				// Подсвеченное совпадение
				int matchStartInLine = qMax(match.startPos, lineStart) - lineStart;
				int matchEndInLine = qMin(match.endPos, lineEnd) - lineStart;

				html += "<span class='match'>";
				html += line.mid(matchStartInLine,
					matchEndInLine - matchStartInLine).toHtmlEscaped();
				html += "</span>";

				lastPos = qMin(match.endPos, lineEnd);
			}
		}

		// Остаток строки
		if (lastPos < lineEnd) {
			html += line.mid(lastPos - lineStart).toHtmlEscaped();
		}

		html += "</p>\n";
		currentPos = lineEnd + 1; // +1 за пропущенный \n
	}

	html += "</body>\n</html>";

	return html;
}

QString MainWindow::generateHighlightedHtml2(const QString& text,
	const QVector<SearchMatch>& matches)
{
	QString html = "<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"    <meta charset='utf-8'>\n"
		"    <style>\n"
		"        body { \n"
		"            font-family: 'Courier New', monospace; \n"
		"            white-space: pre-wrap; \n"
		"            padding: 20px; \n"
		"            background-color: white;\n"
		"            color: black;\n"
		"        }\n"
		"        .match { \n"
		"            background-color: #ffff00; \n"
		"            font-weight: bold; \n"
		"        }\n"
		"        .match-info { \n"
		"            position: fixed; \n"
		"            top: 10px; \n"
		"            right: 10px; \n"
		"            background: white; \n"
		"            padding: 5px 10px; \n"
		"            border: 1px solid #ccc; \n"
		"            border-radius: 3px;\n"
		"            box-shadow: 0 2px 5px rgba(0,0,0,0.1);\n"
		"        }\n"
		"    </style>\n"
		"</head>\n"
		"<body>\n";

	html += QString("<div class='match-info'>Found %1 match%2</div>\n")
		.arg(matches.size())
		.arg(matches.size() == 1 ? "" : "es");

	html += "<pre>";

	int lastPos = 0;
	for (int i = 0; i < matches.size(); ++i) {
		const auto& match = matches[i];

		if (match.startPos < lastPos || match.startPos > text.length() ||
			match.endPos > text.length()) {
			qDebug() << "Invalid match positions:" << match.startPos << match.endPos;
			continue;
		}

		// Текст до совпадения
		html += text.mid(lastPos, match.startPos - lastPos).toHtmlEscaped();

		// Подсвеченное совпадение
		html += "<span class='match'>";
		html += text.mid(match.startPos, match.endPos - match.startPos).toHtmlEscaped();
		html += "</span>";

		lastPos = match.endPos;
	}

	if (lastPos < text.length()) {
		html += text.mid(lastPos).toHtmlEscaped();
	}

	html += "</pre>\n</body>\n</html>";

	return html;
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
