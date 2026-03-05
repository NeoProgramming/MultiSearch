#pragma once

#include <QString>
#include <QByteArray>
#include "ted/text_encoding_detect.h" // Ваша библиотека

class FileExtractor
{
public:
	enum ExtractMode {
		ExtractFull,      // Полный HTML (для отображения)
		ExtractTextOnly   // Только текст (для поиска)
	};

	// Главный метод: загружает файл и возвращает Unicode текст
	static QString loadFile(const QString& filePath, ExtractMode mode = ExtractTextOnly);

	// Вспомогательные методы (можно сделать public для тестирования)
	static QString decodeQuotedPrintable(const QByteArray& data);
	static QString decodeWithEncoding(const QByteArray& data, AutoIt::Common::TextEncodingDetect::Encoding encoding);
	static QByteArray decodeQuotedPrintableToBytes(const QByteArray& data);
private:
	// Обработчики разных форматов
	static QString handleMhtml(const QByteArray& rawData, ExtractMode mode);
	static QString handleHtml(const QByteArray& rawData, const QString& filePath, ExtractMode mode);
	static QString handlePlainText(const QByteArray& rawData);
	static QByteArray FileExtractor::processQuotedLine(const QString& line);

	// Поиск HTML-контента в MHTML
	static QByteArray extractHtmlFromMhtml(const QByteArray& mhtmlData);

	static QString extractPlainTextFromHtml(const QString& html);
};

