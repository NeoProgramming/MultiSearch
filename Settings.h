#pragma once
#include <QString>

struct Settings
{
	// Константы для значений по умолчанию
	static const int DEFAULT_PANEL_WIDTH = 300;
		
	void loadSettings();
	void saveSettings();

	QString searchPath;
	QString searchWords;
	
	QByteArray windowGeometry;
	QByteArray windowState;
	int panelWidth;
};

