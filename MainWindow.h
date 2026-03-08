#pragma once

#include <QtWidgets/QMainWindow>
#include "SearchDock.h"
#include "SearchEngine.h"
#include "Settings.h"
#include <QMap>
#include <QString>
#include <QVector>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
	void onTabCloseRequested(int index);
	void onCurrentTabChanged(int index);
	void onSearchRequested();
	void onFileDoubleClicked(const QString& filePath);
	void onFileOpenAsText();
private:
	void setupUi();
	void createSearchDock();
	void createTabWidget();
	void createTextTab(const QString& title, const QString& text);
	void processFile(const QString& filePath,
		const QString& word1,
		const QString& word2,
		const SearchEngine& searcher,
		int& totalMatches);
	void openFileWithHighlights(const QString& filePath,
		const QVector<SearchMatch>& matches);
	QString generateHighlightedHtml(const QString& text,
		const QVector<SearchMatch>& matches);
	QString generateHighlightedHtml2(const QString& text,
		const QVector<SearchMatch>& matches);

	Settings cfg;
	QTabWidget* m_tabWidget;
	QDockWidget* m_searchDockWidget;
	SearchDock* m_searchDock;
	QAction* m_openAsTextAction;
	QMap<QString, QVector<SearchMatch>> m_fileMatches;
};
