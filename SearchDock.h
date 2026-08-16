#pragma once

#include <QWidget>

class QPushButton;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QTreeView;
class QStandardItemModel;
class QComboBox;

class SearchDock  : public QWidget
{
	Q_OBJECT

public:
	explicit SearchDock(QWidget *parent);
	~SearchDock();

	// Геттеры для доступа к данным из MainWindow
	QString getSearchPath() const;
	QStringList getSearchPaths() const;
	QStringList getSearchWords() const;
	int getSearchRadius() const;
	bool isCaseSensitive() const;
	bool isWholeWords() const;

	// Новые методы для установки значений
	void setSearchPath(const QString& path);
	void setSearchPaths(const QStringList& paths);
		
	void addSearchPath(const QString& path);
	void removeSearchPath(const QString& path);

	void setSearchWords(const QString& words);
	void setSearchRadius(int radius);
	void setCaseSensitive(bool sensitive);
	void setWholeWords(bool wholewords);

	// Методы для работы с результатами
	void addResult(const QString& fileName, const QString& fullPath, int matchCount);
	void addResult(const QString& fileName, const QString& fullPath,
		int matchCount, const QString& context);
	void clearResults();
	
	int getResultCount() const;

signals:
	// Сигнал для запуска поиска
	void searchRequested();
	void fileDoubleClicked(const QString& filePath);
	void searchPathChanged(const QString& path);
	void searchPathRemoved(const QString& path);

private slots:
	void onBrowseFileClicked();
	void onBrowseFolderClicked();
	void onSearchClicked();
	void onResultDoubleClicked(const QModelIndex& index);
	void onPathComboChanged(const QString& text);
	void onRemovePathClicked();
private:
	void setupUi();
	void updateResultsCount();
	void updateRemoveButtonState();

	// Элементы управления
	QComboBox* m_pathCombo;
	QPushButton* m_browseFileButton;
	QPushButton* m_browseFolderButton;
	QPushButton* m_removePathButton;
	QLineEdit* m_wordsEdit;
	QSpinBox* m_radiusSpin;
	QCheckBox* m_caseSensitiveCheck;
	QCheckBox* m_wholeWordsCheck;
	QPushButton* m_searchButton;
	QTreeView* m_resultsView;
	QStandardItemModel* m_resultsModel;
};
