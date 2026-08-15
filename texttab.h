#pragma once

#include <QWidget>

#include <QWidget>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QLabel>
#include <QAction>
#include <QFileInfo>
#include "searchengine.h"

class TextTab : public QWidget
{
	Q_OBJECT

public:
	explicit TextTab(const QString& filePath,
		const QString& text,
		const QVector<SearchMatch>& matches,
		QWidget* parent = nullptr);
	
	QString getFilePath() const { return m_filePath; }
	QString getTabTitle() const { return QFileInfo(m_filePath).fileName(); }
	void setFocusToTextEdit();

private slots:
	void nextMatch();
	void prevMatch();
	void copySelection();
	void findInText();
	void onTabShown();

private:
	void setupUi();
	void highlightMatches();
	void highlightCurrentMatch();
	void updateMatchLabel();
	bool validateMatches();
	
	QString m_filePath;
	QPlainTextEdit* m_textEdit;
	QVector<SearchMatch> m_matches;
	int m_currentMatch;

	QLabel* m_matchLabel;
	QAction* m_prevAction;
	QAction* m_nextAction;
	QAction* m_copyAction;
	QAction* m_findAction;
};
