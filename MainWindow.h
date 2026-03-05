#pragma once

#include <QtWidgets/QMainWindow>
#include "SearchDock.h"

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

	QTabWidget* m_tabWidget;
	QDockWidget* m_searchDockWidget;
	SearchDock* m_searchDock;
	QAction* m_openAsTextAction;
};
