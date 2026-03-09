#pragma once
#include <QString>

struct Settings
{
	
	void loadSettings();
	void saveSettings();

	QString searchPath;
	QString searchWords;
	int searchRadius;
	bool caseSensitive;
	bool wholeWords;
	
	QByteArray windowGeometry;
	QByteArray windowState;
};

