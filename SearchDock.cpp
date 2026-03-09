#include "SearchDock.h"
#include <QTabWidget>
#include <QDockWidget>
#include <QWebEngineView>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QTreeView>
#include <QStandardItemModel>





SearchDock::SearchDock(QWidget *parent)
	: QWidget(parent)
	, m_pathEdit(nullptr)
	, m_browseFileButton(nullptr)
	, m_browseFolderButton(nullptr)
	, m_wordsEdit(nullptr)
	, m_radiusSpin(nullptr)
	, m_caseSensitiveCheck(nullptr)
	, m_wholeWordsCheck(nullptr)
	, m_searchButton(nullptr)
	, m_resultsView(nullptr)
	, m_resultsModel(nullptr)
{
	setupUi();
}

SearchDock::~SearchDock()
{}

void SearchDock::setupUi()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(10);

	// === Группа "Путь для поиска" ===
	QGroupBox* pathGroup = new QGroupBox("Search Path", this);
	QVBoxLayout* pathLayout = new QVBoxLayout(pathGroup);

	// Поле для пути
	m_pathEdit = new QLineEdit(this);
	m_pathEdit->setPlaceholderText("Enter file or folder path...");

	// Ряд с кнопками
	QHBoxLayout* buttonLayout = new QHBoxLayout;
	m_browseFileButton = new QPushButton("Select File...", this);
	m_browseFolderButton = new QPushButton("Select Folder...", this);

	buttonLayout->addWidget(m_browseFileButton);
	buttonLayout->addWidget(m_browseFolderButton);
	buttonLayout->addStretch();

	pathLayout->addWidget(m_pathEdit);
	pathLayout->addLayout(buttonLayout);
	pathGroup->setLayout(pathLayout);

	// === Группа "Параметры поиска" ===
	QGroupBox* searchGroup = new QGroupBox("Search Options", this);
	QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);

	// Поле ввода слов
	QLabel* wordsLabel = new QLabel("Words to find:", this);
	m_wordsEdit = new QLineEdit(this);
	m_wordsEdit->setPlaceholderText("Enter words separated by space...");

	// Радиус поиска
	QHBoxLayout* radiusLayout = new QHBoxLayout;
	QLabel* radiusLabel = new QLabel("Max distance:", this);
	m_radiusSpin = new QSpinBox(this);
	m_radiusSpin->setRange(10, 500);
	m_radiusSpin->setValue(50);
	m_radiusSpin->setSuffix(" chars");
	radiusLayout->addWidget(radiusLabel);
	radiusLayout->addWidget(m_radiusSpin);
	radiusLayout->addStretch();

	// Чекбоксы
	m_caseSensitiveCheck = new QCheckBox("Case sensitive", this);
	m_wholeWordsCheck = new QCheckBox("Whole words only", this);

	// Кнопка поиска
	m_searchButton = new QPushButton("Search", this);
	m_searchButton->setStyleSheet("QPushButton { font-weight: bold; }");

	searchLayout->addWidget(wordsLabel);
	searchLayout->addWidget(m_wordsEdit);
	searchLayout->addLayout(radiusLayout);
	searchLayout->addWidget(m_caseSensitiveCheck);
	searchLayout->addWidget(m_wholeWordsCheck);
	searchLayout->addWidget(m_searchButton);

	// === Группа "Результаты" ===
	QGroupBox* resultsGroup = new QGroupBox("Results", this);
	QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);

	m_resultsView = new QTreeView(this);
	m_resultsModel = new QStandardItemModel(this);
	m_resultsModel->setHorizontalHeaderLabels({ "File", "Matches" });
	m_resultsView->setModel(m_resultsModel);
	m_resultsView->setAlternatingRowColors(true);
	m_resultsView->setSortingEnabled(true);
	//m_resultsView->header()->setStretchLastSection(true);
	m_resultsView->setRootIsDecorated(false);
	// Настройка растягивания колонок
	m_resultsView->header()->setStretchLastSection(false); // не растягивать последнюю
	// Первая колонка (File) - растягивается
	m_resultsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	// Вторая колонка (Matches) - фиксированная ширина
	m_resultsView->header()->setSectionResizeMode(1, QHeaderView::Fixed);
	m_resultsView->setColumnWidth(1, 50);

	resultsLayout->addWidget(m_resultsView);

	// Собираем всё вместе
	mainLayout->addWidget(pathGroup);
	mainLayout->addWidget(searchGroup);
	mainLayout->addWidget(resultsGroup);

	// Подключаем сигналы
	connect(m_browseFileButton, &QPushButton::clicked, this, &SearchDock::onBrowseFileClicked);
	connect(m_browseFolderButton, &QPushButton::clicked, this, &SearchDock::onBrowseFolderClicked);
	connect(m_searchButton, &QPushButton::clicked, this, &SearchDock::onSearchClicked);
	connect(m_resultsView, &QTreeView::doubleClicked, this, &SearchDock::onResultDoubleClicked);

	setLayout(mainLayout);

	// Обновляем заголовки колонок
	m_resultsModel->setHorizontalHeaderLabels({ "File", "Matches", "Context" });

	// Скрываем колонку с контекстом для компактности? Или показываем
	m_resultsView->hideColumn(2); // Прячем контекст, показываем при клике
}

void SearchDock::onBrowseFileClicked()
{
	QString filePath = QFileDialog::getOpenFileName(
		this,
		"Select File",
		QString(),
		"All Files (*.*);;HTML Files (*.html *.htm);;Text Files (*.txt);;MHTML Files (*.mht *.mhtml)"
	);

	if (!filePath.isEmpty()) {
		m_pathEdit->setText(filePath);
	}
}

void SearchDock::onBrowseFolderClicked()
{
	QString folderPath = QFileDialog::getExistingDirectory(
		this,
		"Select Folder",
		QString(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
	);

	if (!folderPath.isEmpty()) {
		m_pathEdit->setText(folderPath);
	}
}

void SearchDock::onSearchClicked()
{
	emit searchRequested();
}

void SearchDock::onResultDoubleClicked(const QModelIndex& index)
{
	if (!index.isValid()) return;

	// Получаем полный путь из данных модели (сохраняем в UserRole)
	QString filePath = m_resultsModel->data(index.sibling(index.row(), 0), Qt::UserRole).toString();
	if (!filePath.isEmpty()) {
		emit fileDoubleClicked(filePath);
	}
}

// Геттеры
QString SearchDock::getSearchPath() const
{
	return m_pathEdit->text().trimmed();
}

QStringList SearchDock::getSearchWords() const
{
	return m_wordsEdit->text().split(' ', Qt::SkipEmptyParts);
}

int SearchDock::getSearchRadius() const
{
	return m_radiusSpin->value();
}

bool SearchDock::isCaseSensitive() const
{
	return m_caseSensitiveCheck->isChecked();
}

bool SearchDock::isWholeWords() const
{
	return m_wholeWordsCheck->isChecked();
}

void SearchDock::setSearchPath(const QString& path)
{
	m_pathEdit->setText(path);
}

void SearchDock::setSearchWords(const QString& words)
{
	m_wordsEdit->setText(words);
}

void SearchDock::setSearchRadius(int radius)
{
	m_radiusSpin->setValue(radius);
}

void SearchDock::setCaseSensitive(bool sensitive)
{
	m_caseSensitiveCheck->setChecked(sensitive);
}

void SearchDock::setWholeWords(bool wholewords) 
{
	m_wholeWordsCheck->setChecked(wholewords);
}

// Методы для работы с результатами
void SearchDock::addResult(const QString& fileName, const QString& fullPath, int matchCount)
{
	QList<QStandardItem*> row;

	QStandardItem* nameItem = new QStandardItem(fileName);
	nameItem->setData(fullPath, Qt::UserRole); // Сохраняем полный путь
	nameItem->setEditable(false);

	QStandardItem* countItem = new QStandardItem(QString::number(matchCount));
	countItem->setTextAlignment(Qt::AlignCenter);
	countItem->setEditable(false);

	row << nameItem << countItem;
	m_resultsModel->appendRow(row);
}

void SearchDock::addResult(const QString& fileName, const QString& fullPath,
	int matchCount, const QString& context)
{
	QList<QStandardItem*> row;

	QStandardItem* nameItem = new QStandardItem(fileName);
	nameItem->setData(fullPath, Qt::UserRole);
	nameItem->setEditable(false);

	QStandardItem* countItem = new QStandardItem(QString::number(matchCount));
	countItem->setTextAlignment(Qt::AlignCenter);
	countItem->setEditable(false);

	QStandardItem* contextItem = new QStandardItem(context);
	contextItem->setEditable(false);

	row << nameItem << countItem << contextItem;
	m_resultsModel->appendRow(row);

	updateResultsCount();
}

void SearchDock::updateResultsCount()
{
	int count = m_resultsModel->rowCount();
	QString title = "Results";
	if (count > 0) {
		title = QString("Results (%1)").arg(count);
	}

	// Находим группу Results и обновляем ее заголовок
	// Для простоты можно просто установить текст где-то еще
}

int SearchDock::getResultCount() const
{
	return m_resultsModel->rowCount();
}



void SearchDock::clearResults()
{
	m_resultsModel->removeRows(0, m_resultsModel->rowCount());
}
