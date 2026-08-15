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
	restoreUi();

	// Настройки окна
	setWindowTitle("MultiSearch");
	resize(1200, 800);

}

void MainWindow::restoreUi()
{
	// Восстанавливаем геометрию и состояние окна
	if (!cfg.windowGeometry.isEmpty()) {
		restoreGeometry(cfg.windowGeometry);
	}

	if (!cfg.windowState.isEmpty()) {
		restoreState(cfg.windowState);
	}

	m_searchDock->setSearchPath(cfg.searchPath);
	m_searchDock->setSearchWords(cfg.searchWords);

	m_searchDock->setSearchRadius(cfg.searchRadius);
	m_searchDock->setCaseSensitive(cfg.caseSensitive);
	m_searchDock->setWholeWords(cfg.wholeWords);

	qDebug() << "UI restored";
}

MainWindow::~MainWindow()
{
	// Сохраняем геометрию и состояние окна
	cfg.windowGeometry = saveGeometry();
	cfg.windowState = saveState();

	cfg.searchPath = m_searchDock->getSearchPath();
	cfg.searchWords = m_searchDock->getSearchWords().join(" ");
	cfg.searchRadius = m_searchDock->getSearchRadius();
	cfg.caseSensitive = m_searchDock->isCaseSensitive();
	cfg.wholeWords = m_searchDock->isWholeWords();

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

	QApplication::setOverrideCursor(Qt::WaitCursor);

	statusBar()->showMessage(QString("Searching for '%1' in %2...")
		.arg(words.join(" "))
		.arg(path), 0);
		

	// Очищаем предыдущие результаты
	m_searchDock->clearResults();
	m_fileMatches.clear();
	m_fileTexts.clear();

	// Настройки поиска
	SearchEngine::Config config;
	config.caseSensitive = m_searchDock->isCaseSensitive();
	config.radius = m_searchDock->getSearchRadius();
	SearchEngine searcher(config);

	QFileInfo pathInfo(path);
	int totalMatches = 0;

	// Определяем режим поиска
	bool isSingleWord = (words.size() == 1);
	QString word1 = words[0];
	QString word2 = isSingleWord ? QString() : words[1];

	if (pathInfo.isFile()) {
		// Поиск в одном файле
		processFile(path, word1, word2, searcher, totalMatches, isSingleWord);
	}
	else if (pathInfo.isDir()) {
		// Поиск в директории рекурсивно
		QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
		int fileCount = 0;

		while (it.hasNext()) {
			QString filePath = it.next();
			// Фильтруем только текстовые файлы
			if (filePath.endsWith(".txt", Qt::CaseInsensitive)) 			
			{
				processFile(filePath, word1, word2, searcher, totalMatches, isSingleWord);

				fileCount++;
				if (fileCount % 10 == 0) {
					statusBar()->showMessage(QString("Processed %1 files...")
						.arg(fileCount));
					QApplication::processEvents();
				}
			}
		}
	}

	QApplication::restoreOverrideCursor();
	statusBar()->showMessage(QString("Search completed. Found %1 matches in %2 files.")
		.arg(totalMatches)
		.arg(m_searchDock->getResultCount()), 5000);
}

void MainWindow::processFile(const QString& filePath,
	const QString& word1,
	const QString& word2,
	const SearchEngine& searcher,
	int& totalMatches,
	bool isSingleWord)
{
	qDebug() << "Processing file:" << filePath;

	QString text = FileExtractor::loadFile(filePath, FileExtractor::ExtractTextOnly);
	if (text.isEmpty()) {
		qDebug() << "File is empty or could not be loaded";
		return;
	}

	QVector<SearchMatch> matches;
	if (isSingleWord) {
		// Поиск одного слова
		matches = searcher.findOneWord(text, word1);
	}
	else {
		// Поиск двух слов
		matches = searcher.findTwoWords(text, word1, word2);
	}


	if (!matches.isEmpty()) {
		QFileInfo fileInfo(filePath);
		QString context = matches.first().surroundingText;
		m_searchDock->addResult(fileInfo.fileName(), filePath,
			matches.size(), context);
		totalMatches += matches.size();

		// Сохраняем matches
		m_fileMatches[filePath] = matches;
		m_fileTexts[filePath] = text; 

		qDebug() << "Saved" << matches.size() << "matches for" << filePath;
		// Для отладки выведем первые несколько позиций
		for (int i = 0; i < qMin(3, matches.size()); ++i) {
			qDebug() << "  Match" << i << ":" << matches[i].startPos << "-" << matches[i].endPos;
		}
	}
}

void MainWindow::onFileDoubleClicked(const QString& filePath)
{
	if (!m_fileMatches.contains(filePath)) {
		qDebug() << "No match data for file:" << filePath;

		// Показываем файл без подсветки
		QFileInfo fileInfo(filePath);
		QWebEngineView* view = new QWebEngineView();
		view->setUrl(QUrl::fromLocalFile(filePath));
		view->setProperty("filePath", filePath);

		int index = m_tabWidget->addTab(view, fileInfo.fileName());
		m_tabWidget->setCurrentIndex(index);
		m_openTabs[filePath] = index;
		return;
	}

	const auto& matches = m_fileMatches[filePath];
	if (matches.isEmpty()) {
		// Файл есть в списке, но совпадений нет (может быть)
		return;
	}

	// Открываем с подсветкой
	openFileWithHighlights(filePath, matches);
}

/*
void MainWindow::onFileDoubleClicked(const QString& filePath)
{
	qDebug() << "\n=== Double click ===";
	qDebug() << "File path:" << filePath;

	// Проверяем, есть ли такой файл в сохраненных результатах
	if (!m_fileMatches.contains(filePath)) {
		qDebug() << "ERROR: File not found in m_fileMatches!";
		qDebug() << "Available files:" << m_fileMatches.keys();

		statusBar()->showMessage("No match data for this file (maybe search was cleared?)", 3000);
		return;
	}

	const auto& matches = m_fileMatches[filePath];
	qDebug() << "Matches count:" << matches.size();

	if (matches.isEmpty()) {
		qDebug() << "ERROR: Matches list is empty!";
		statusBar()->showMessage("No matches found in this file", 3000);
		return;
	}

	// Проверяем первый match на валидность
	const auto& firstMatch = matches.first();
	qDebug() << "First match - start:" << firstMatch.startPos
		<< "end:" << firstMatch.endPos;

	// Открываем файл с подсветкой
	openFileWithHighlights(filePath, matches);
}*/

void MainWindow::openFileWithHighlights(const QString& filePath,
	const QVector<SearchMatch>& matches)
{
	// Проверяем, не открыт ли уже этот файл
	int existingTabIndex = findOpenTab(filePath);

	if (existingTabIndex != -1) {
		// Файл уже открыт - просто переключаемся на вкладку
		switchToTab(existingTabIndex);

		// Обновляем подсветку, если нужно (например, если поиск был обновлен)
		// Но можно пропустить, так как пользователь уже видел этот файл

		statusBar()->showMessage(QString("Switched to '%1'").arg(QFileInfo(filePath).fileName()), 2000);
		return;
	}

	// Файл не открыт - создаем новую вкладку
	addNewTab(filePath, matches);
}

int MainWindow::findOpenTab(const QString& filePath)
{
	// Проверяем по кэшу
	if (m_openTabs.contains(filePath)) {
		int index = m_openTabs[filePath];

		// Проверяем, что вкладка действительно существует
		if (index >= 0 && index < m_tabWidget->count()) {
			// Проверяем, что это действительно тот же файл
			QWidget* widget = m_tabWidget->widget(index);
			if (widget) {
				// Получаем путь из сохраненных данных
				// Для TextTab
				TextTab* textTab = qobject_cast<TextTab*>(widget);
				if (textTab && textTab->getFilePath() == filePath) {
					return index;
				}

				// Для QWebEngineView (HTML/MHTML)
				QWebEngineView* webView = qobject_cast<QWebEngineView*>(widget);
				if (webView) {
					// Для QWebEngineView сложнее получить путь,
					// можно хранить в свойстве
					QString tabPath = webView->property("filePath").toString();
					if (tabPath == filePath) {
						return index;
					}
				}
			}
		}

		// Если вкладка не найдена - удаляем из кэша
		m_openTabs.remove(filePath);
	}

	// Если не нашли по кэшу - ищем перебором
	for (int i = 0; i < m_tabWidget->count(); ++i) {
		QWidget* widget = m_tabWidget->widget(i);

		// Проверяем TextTab
		TextTab* textTab = qobject_cast<TextTab*>(widget);
		if (textTab && textTab->getFilePath() == filePath) {
			m_openTabs[filePath] = i; // обновляем кэш
			return i;
		}

		// Проверяем QWebEngineView
		QWebEngineView* webView = qobject_cast<QWebEngineView*>(widget);
		if (webView) {
			QString tabPath = webView->property("filePath").toString();
			if (tabPath == filePath) {
				m_openTabs[filePath] = i; // обновляем кэш
				return i;
			}
		}
	}

	return -1;
}

void MainWindow::switchToTab(int index)
{
	if (index >= 0 && index < m_tabWidget->count()) {
		m_tabWidget->setCurrentIndex(index);

		// Устанавливаем фокус на содержимое вкладки
		QWidget* widget = m_tabWidget->widget(index);
		if (widget) {
			// Для TextTab
			TextTab* textTab = qobject_cast<TextTab*>(widget);
			if (textTab) {
				textTab->setFocusToTextEdit();
			}

			// Для QWebEngineView - фокус устанавливается автоматически
		}

		//m_tabWidget->widget(index)->setFocus();
	}
}

void MainWindow::addNewTab(const QString& filePath,
	const QVector<SearchMatch>& matches)
{
	QFileInfo fileInfo(filePath);
	QString extension = fileInfo.suffix().toLower();

	bool isHtml = (extension == "html" || extension == "htm");
	bool isMhtml = (extension == "mht" || extension == "mhtml");

	int newIndex = -1;

	if (isHtml || isMhtml) {
		// Для HTML/MHTML используем QWebEngine
		QWebEngineView* view = new QWebEngineView();

		// Сохраняем путь в свойстве для идентификации
		view->setProperty("filePath", filePath);

		QString text = FileExtractor::loadFile(filePath, FileExtractor::ExtractFull);

		if (text.isEmpty()) {
			view->setHtml("<html><body><h2>Error loading file</h2></body></html>");
		}
		else {
			QString html = generateHighlightedHtml(text, matches);

			// Для больших HTML файлов используем временный файл
			if (html.length() > 1024 * 1024) { // > 1MB
				QString tempDir = QDir::temp().absoluteFilePath("multisearch");
				QDir().mkpath(tempDir);

				QString tempFilePath = tempDir + "/" + fileInfo.fileName() + "_" +
					QString::number(QDateTime::currentMSecsSinceEpoch()) + ".html";

				QFile tempFile(tempFilePath);
				if (tempFile.open(QIODevice::WriteOnly)) {
					tempFile.write(html.toUtf8());
					tempFile.close();
					view->setUrl(QUrl::fromLocalFile(tempFilePath));
				}
				else {
					view->setHtml(html);
				}
			}
			else {
				view->setHtml(html);
			}
		}

		newIndex = m_tabWidget->addTab(view, fileInfo.fileName());

	}
	else {
		// Для текстовых файлов используем TextTab
		if (!m_fileTexts.contains(filePath)) {
			qWarning() << "No saved text for" << filePath;
			return;
		}

		QString text = m_fileTexts[filePath];
		TextTab* tab = new TextTab(filePath, text, matches);

		newIndex = m_tabWidget->addTab(tab, fileInfo.fileName());
	}

	if (newIndex >= 0) {
		// Сохраняем в кэш
		m_openTabs[filePath] = newIndex;
		m_tabWidget->setCurrentIndex(newIndex);

		// Устанавливаем фокус на новую вкладку
		QWidget* widget = m_tabWidget->widget(newIndex);
		if (widget) {
			TextTab* textTab = qobject_cast<TextTab*>(widget);
			if (textTab) {
				textTab->setFocusToTextEdit();
			}
		}

		// Подключаем обработчик закрытия вкладки для очистки кэша
		connect(m_tabWidget, &QTabWidget::tabCloseRequested,
			this, &MainWindow::onTabCloseRequested);
	}
}
/*
void MainWindow::onTabCloseRequested(int index)
{
	if (index < 0 || index >= m_tabWidget->count()) return;

	// Не закрываем последнюю вкладку
	if (m_tabWidget->count() <= 1) {
		return;
	}

	QWidget* widget = m_tabWidget->widget(index);

	// Удаляем из кэша
	QString filePathToRemove;
	for (auto it = m_openTabs.begin(); it != m_openTabs.end(); ++it) {
		if (it.value() == index) {
			filePathToRemove = it.key();
			break;
		}
	}

	if (!filePathToRemove.isEmpty()) {
		m_openTabs.remove(filePathToRemove);
	}

	// Удаляем вкладку
	m_tabWidget->removeTab(index);
	delete widget;

	// Обновляем индексы в кэше для вкладок после удаленной
	for (auto it = m_openTabs.begin(); it != m_openTabs.end(); ++it) {
		if (it.value() > index) {
			it.value()--;
		}
	}
}


void MainWindow::openFileWithHighlights(const QString& filePath,
	const QVector<SearchMatch>& matches)
{
	qDebug() << "\n=== openFileWithHighlights ===";
	qDebug() << "File path:" << filePath;
	qDebug() << "Matches count:" << matches.size();

	QFileInfo fileInfo(filePath);

	// Проверяем существование файла
	if (!fileInfo.exists()) {
		qDebug() << "ERROR: File does not exist!";
		statusBar()->showMessage("File does not exist: " + filePath, 3000);
		return;
	}

	qDebug() << "File size:" << fileInfo.size();
	qDebug() << "File suffix:" << fileInfo.suffix();

	// Загружаем текст файла для проверки
	QString text = FileExtractor::loadFile(filePath, FileExtractor::ExtractFull);
	qDebug() << "Loaded text length:" << text.length();

	if (text.isEmpty()) {
		qDebug() << "ERROR: Loaded text is empty!";
		statusBar()->showMessage("File is empty or could not be read", 3000);

		// Показываем заглушку вместо падения
		QWebEngineView* view = new QWebEngineView();
		view->setHtml("<html><body><h2>Error: Empty file or cannot read</h2>"
			"<p>File: " + filePath + "</p></body></html>");
		int index = m_tabWidget->addTab(view, fileInfo.fileName());
		m_tabWidget->setCurrentIndex(index);
		return;
	}

	// Проверяем matches на валидность
	QVector<SearchMatch> validMatches;
	for (const auto& match : matches) {
		if (match.startPos >= 0 && match.endPos <= text.length() &&
			match.startPos < match.endPos) {
			validMatches.append(match);
		}
		else {
			qDebug() << "WARNING: Invalid match - start:" << match.startPos
				<< "end:" << match.endPos << "text length:" << text.length();
		}
	}

	qDebug() << "Valid matches count:" << validMatches.size();

	if (validMatches.isEmpty()) {
		qDebug() << "ERROR: No valid matches!";
		statusBar()->showMessage("No valid matches found in file", 3000);

		// Показываем файл без подсветки
		QWebEngineView* view = new QWebEngineView();
		view->setHtml("<html><body><pre>" + text.toHtmlEscaped() + "</pre></body></html>");
		int index = m_tabWidget->addTab(view, fileInfo.fileName());
		m_tabWidget->setCurrentIndex(index);
		return;
	}

	QString extension = fileInfo.suffix().toLower();
	bool isHtml = (extension == "html" || extension == "htm");
	bool isMhtml = (extension == "mht" || extension == "mhtml");

	try {
		if (isHtml || isMhtml) {
			qDebug() << "Opening as HTML/MHTML";
			openHtmlWithHighlights(filePath, validMatches, text);
		}
		else {
			qDebug() << "Opening as text";
			openTextWithHighlights(filePath, validMatches, text);
		}
	}
	catch (const std::exception& e) {
		qDebug() << "EXCEPTION:" << e.what();
		statusBar()->showMessage("Error opening file: " + QString(e.what()), 3000);
	}
	catch (...) {
		qDebug() << "UNKNOWN EXCEPTION";
		statusBar()->showMessage("Unknown error opening file", 3000);
	}
}
*/

void MainWindow::openHtmlWithHighlights(const QString& filePath,
	const QVector<SearchMatch>& matches,
	const QString& text)
{
	QWebEngineView* view = new QWebEngineView();
	QFileInfo fileInfo(filePath);

	QString html = generateHighlightedHtml(text, matches);

	// Для больших HTML файлов используем временный файл
	if (html.length() > 1024 * 1024) { // > 1MB
		QString tempDir = QDir::temp().absoluteFilePath("multisearch");
		QDir().mkpath(tempDir);

		QString tempFilePath = tempDir + "/" + fileInfo.fileName() + "_" +
			QString::number(QDateTime::currentMSecsSinceEpoch()) + ".html";

		QFile tempFile(tempFilePath);
		if (tempFile.open(QIODevice::WriteOnly)) {
			tempFile.write(html.toUtf8());
			tempFile.close();
			qDebug() << "Saved temp HTML to:" << tempFilePath;
			view->setUrl(QUrl::fromLocalFile(tempFilePath));
		}
		else {
			qDebug() << "Failed to create temp file, using setHtml";
			view->setHtml(html, QUrl::fromLocalFile(fileInfo.absolutePath() + "/"));
		}
	}
	else {
		view->setHtml(html, QUrl::fromLocalFile(fileInfo.absolutePath() + "/"));
	}

	// Подключаем отладку загрузки
	connect(view, &QWebEngineView::loadFinished, this,
		[filePath](bool ok) {
		qDebug() << "WebEngine load finished for" << filePath << "- ok:" << ok;
	});

	int index = m_tabWidget->addTab(view, fileInfo.fileName());
	m_tabWidget->setCurrentIndex(index);
}

void MainWindow::openTextWithHighlights(const QString& filePath,
	const QVector<SearchMatch>& matches,
	const QString& text)
{
	TextTab* tab = new TextTab(filePath, text, matches);

	QFileInfo fileInfo(filePath);
	int index = m_tabWidget->addTab(tab, fileInfo.fileName());
	m_tabWidget->setCurrentIndex(index);
}

QString MainWindow::generateHighlightedHtml(const QString& text,
	const QVector<SearchMatch>& matches)
{
	if (text.isEmpty()) {
		return "<html><body>Empty document</body></html>";
	}

	QString html = "<!DOCTYPE html><html><head><meta charset='utf-8'><style>";
	html += "body{font-family:monospace;white-space:pre-wrap;padding:20px}";
	html += ".match{background:#ff0}</style></head><body>";

	int lastPos = 0;
	for (int i = 0; i < matches.size(); ++i) {
		const auto& match = matches[i];

		// Защита от некорректных позиций
		if (match.startPos < lastPos || match.startPos > text.length() ||
			match.endPos > text.length() || match.startPos >= match.endPos) {
			qDebug() << "Skipping invalid match:" << match.startPos << match.endPos;
			continue;
		}

		// Текст до совпадения
		if (match.startPos > lastPos) {
			html += text.mid(lastPos, match.startPos - lastPos).toHtmlEscaped();
		}

		// Подсвеченное совпадение
		html += "<span class='match'>";
		html += text.mid(match.startPos, match.endPos - match.startPos).toHtmlEscaped();
		html += "</span>";

		lastPos = match.endPos;
	}

	// Остаток текста
	if (lastPos < text.length()) {
		html += text.mid(lastPos).toHtmlEscaped();
	}

	html += "</body></html>";
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
