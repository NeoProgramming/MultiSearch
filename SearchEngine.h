#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>

struct SearchMatch {
	int startPos;           // позиция начала совпадения в тексте
	int endPos;             // позиция конца совпадения
	int word1Pos;           // позиция первого слова
	int word2Pos;           // позиция второго слова
	QString surroundingText; // текст вокруг для контекста
};

class SearchEngine
{
public:
	struct Config {
		bool caseSensitive = false;
		int radius = 50;        // максимальное расстояние между словами
		bool wholeWords = false; // только целые слова (пока не реализуем)
	};

	SearchEngine(const Config& config = Config());

	// Поиск двух слов в тексте
	QVector<SearchMatch> findTwoWords(const QString& text,
		const QString& word1,
		const QString& word2) const;

	// Утилита для получения контекста вокруг совпадения
	static QString extractContext(const QString& text, int pos, int contextSize = 50);

private:
	Config m_config;

	// Поиск всех позиций слова в тексте
	QVector<int> findAllPositions(const QString& text, const QString& word) const;

	// Нормализация строки (приведение к нижнему регистру если нужно)
	QString normalize(const QString& str) const;
};

