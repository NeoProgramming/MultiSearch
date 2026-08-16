#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariant>

#define SETTINGS_LIST	\
	X(QString,		searchPath,		"path", "")\
	X(QStringList,	searchPaths,	"paths", QVariant())\
	X(QString,		searchWords,	"words", "")\
	X(QByteArray,	windowGeometry,	"win_geometry", QVariant())\
	X(QByteArray,	windowState,	"win_state", QVariant())\
	X(int,			searchRadius,	"radius", 20)\
	X(bool,			caseSensitive,	"case_sensitive", false)\
	X(bool,			wholeWords,		"whole_words", false)

struct Settings
{	
	void loadSettings();
	void saveSettings();
	
#define X(type, var, id, def)	type var;
	SETTINGS_LIST
#undef X
	
};

