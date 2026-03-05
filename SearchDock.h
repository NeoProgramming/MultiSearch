#pragma once

#include <QWidget>

class QPushButton;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QTreeView;
class QStandardItemModel;

class SearchDock  : public QWidget
{
	Q_OBJECT

public:
	explicit SearchDock(QWidget *parent);
	~SearchDock();

	// √еттеры дл€ доступа к данным из MainWindow
	QString getSearchPath() const;
	QStringList getSearchWords() const;
	int getSearchRadius() const;
	bool isCaseSensitive() const;
	bool isWholeWords() const;

	// ћетоды дл€ работы с результатами
	void addResult(const QString& fileName, const QString& fullPath, int matchCount);
	void clearResults();

signals:
	// —игнал дл€ запуска поиска
	void searchRequested();
	void fileDoubleClicked(const QString& filePath);

private slots:
	void onBrowseFileClicked();
	void onBrowseFolderClicked();
	void onSearchClicked();
	void onResultDoubleClicked(const QModelIndex& index);

private:
	void setupUi();

	// Ёлементы управлени€
	QLineEdit* m_pathEdit;
	QPushButton* m_browseFileButton;
	QPushButton* m_browseFolderButton;
	QLineEdit* m_wordsEdit;
	QSpinBox* m_radiusSpin;
	QCheckBox* m_caseSensitiveCheck;
	QCheckBox* m_wholeWordsCheck;
	QPushButton* m_searchButton;
	QTreeView* m_resultsView;
	QStandardItemModel* m_resultsModel;
};
