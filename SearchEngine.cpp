#include "SearchEngine.h"

#include "searchengine.h"
#include <QtAlgorithms>

SearchEngine::SearchEngine(const Config& config)
	: m_config(config)
{
}

QString SearchEngine::normalize(const QString& str) const
{
	if (m_config.caseSensitive) {
		return str;
	}
	return str.toLower();
}

QVector<int> SearchEngine::findAllPositions(const QString& text, const QString& word) const
{
	QVector<int> positions;

	if (word.isEmpty() || text.isEmpty()) {
		return positions;
	}

	QString searchText = normalize(text);
	QString searchWord = normalize(word);

	int pos = 0;
	while ((pos = searchText.indexOf(searchWord, pos)) != -1) {
		positions.append(pos);
		pos++; // Сдвигаем на 1, чтобы найти пересекающиеся совпадения
	}

	return positions;
}

QVector<SearchMatch> SearchEngine::findTwoWords(const QString& text,
	const QString& word1,
	const QString& word2) const
{
	QVector<SearchMatch> results;

	if (word1.isEmpty() || word2.isEmpty() || text.isEmpty()) {
		return results;
	}

	// Находим все позиции обоих слов
	QVector<int> pos1 = findAllPositions(text, word1);
	QVector<int> pos2 = findAllPositions(text, word2);

	if (pos1.isEmpty() || pos2.isEmpty()) {
		return results;
	}

	int len1 = word1.length();
	int len2 = word2.length();

	// Для каждой позиции первого слова ищем подходящее второе
	for (int p1 : pos1) {
		for (int p2 : pos2) {
			// Вычисляем расстояние между словами
			int distance = 0;
			int startPos = 0;
			int endPos = 0;

			if (p1 < p2) {
				// слово1 перед словом2
				distance = p2 - (p1 + len1);
				startPos = p1;
				endPos = p2 + len2;
			}
			else {
				// слово2 перед словом1
				distance = p1 - (p2 + len2);
				startPos = p2;
				endPos = p1 + len1;
			}

			// Проверяем радиус
			if (distance <= m_config.radius) {
				SearchMatch match;
				match.startPos = startPos;
				match.endPos = endPos;
				match.word1Pos = p1;
				match.word2Pos = p2;
				match.surroundingText = extractContext(text, startPos, 50);

				results.append(match);
			}
		}
	}

	// Удаляем дубликаты (если оба слова одинаковые)
	// Простая сортировка и уникальность
	std::sort(results.begin(), results.end(),
		[](const SearchMatch& a, const SearchMatch& b) {
		if (a.startPos != b.startPos) return a.startPos < b.startPos;
		return a.endPos < b.endPos;
	});

	auto last = std::unique(results.begin(), results.end(),
		[](const SearchMatch& a, const SearchMatch& b) {
		return a.startPos == b.startPos && a.endPos == b.endPos;
	});
	results.erase(last, results.end());

	return results;
}

QString SearchEngine::extractContext(const QString& text, int pos, int contextSize)
{
	int start = qMax(0, pos - contextSize);
	int end = qMin(text.length(), pos + contextSize);

	QString context = text.mid(start, end - start);

	// Добавляем многоточия если нужно
	if (start > 0) context = "..." + context;
	if (end < text.length()) context = context + "...";

	// Заменяем переносы строк на пробелы для компактности
	context.replace('\n', ' ').replace('\r', ' ');

	return context;
}
