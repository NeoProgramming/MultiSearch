#include "FileExtractor.h"

#include <QFile>
#include <QTextCodec>
#include <QDebug>
#include <QRegularExpression>

QString FileExtractor::loadFile(const QString& filePath, ExtractMode mode)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Cannot open file:" << filePath;
		return QString();
	}

	QByteArray rawData = file.readAll();
	file.close();

	// Определяем тип файла по расширению
	if (filePath.endsWith(".mht", Qt::CaseInsensitive) ||
		filePath.endsWith(".mhtml", Qt::CaseInsensitive)) {
		return handleMhtml(rawData, mode);
	}
	else if (filePath.endsWith(".html", Qt::CaseInsensitive) ||
		filePath.endsWith(".htm", Qt::CaseInsensitive)) {
		return handleHtml(rawData, filePath, mode);
	}
	else {
		// Все остальное (txt, без расширения, и т.д.)
		return handlePlainText(rawData);
	}
}

QString FileExtractor::handleMhtml(const QByteArray& rawData, ExtractMode mode)
{
	// 1. Извлекаем HTML-часть из MHTML контейнера
	QByteArray htmlPart = extractHtmlFromMhtml(rawData);
	if (htmlPart.isEmpty()) {
		qWarning() << "No HTML content found in MHTML file";
		return QString();
	}

	// 2. Декодируем quoted-printable, получаем сырые байты
	QByteArray decodedBytes = decodeQuotedPrintableToBytes(htmlPart);

	// 3. Теперь обрабатываем как HTML (определяем кодировку)
	return handleHtml(decodedBytes, "dummy.html", mode);
}

// Новый метод - декодирует quoted-printable в QByteArray, а не в QString
QByteArray FileExtractor::decodeQuotedPrintableToBytes(const QByteArray& data)
{
	QByteArray result;
	result.reserve(data.size());

	QString text = QString::fromLatin1(data);
	QStringList lines = text.split('\n');

	for (int i = 0; i < lines.size(); ++i) {
		QString line = lines[i];

		// Убираем \r если есть
		if (line.endsWith('\r')) {
			line.chop(1);
		}

		// Проверяем, есть ли мягкий перенос в конце строки
		bool softBreak = line.endsWith('=');

		if (softBreak) {
			// Убираем = в конце и добавляем без переноса строки
			line.chop(1);
			result.append(processQuotedLine(line));
		}
		else {
			// Обычная строка - добавляем с переносом
			result.append(processQuotedLine(line));
			if (i < lines.size() - 1) { // Не добавляем \n после последней строки
				result.append('\n');
			}
		}
	}

	return result;
}

QByteArray FileExtractor::processQuotedLine(const QString& line)
{
	QByteArray result;
	result.reserve(line.length());

	for (int i = 0; i < line.length(); ++i) {
		QChar ch = line[i];

		if (ch == '=' && i + 2 < line.length()) {
			// Декодируем =XX
			QString hex = line.mid(i + 1, 2);
			bool ok;
			int value = hex.toInt(&ok, 16);

			if (ok) {
				result.append(static_cast<char>(value));
				i += 2;
			}
			else {
				// Неправильная hex последовательность - оставляем как есть
				result.append('=');
			}
		}
		else {
			// Обычный символ
			if (ch.toLatin1() != 0) { // Проверяем, что символ в ASCII диапазоне
				result.append(ch.toLatin1());
			}
			else {
				// Не-ASCII символ (должен быть закодирован через =XX)
				// Если мы сюда попали, значит что-то не так с кодировкой
				result.append('?');
			}
		}
	}

	return result;
}

QString FileExtractor::handleHtml(const QByteArray& rawData, const QString& filePath, ExtractMode mode)
{
	// Определяем кодировку и получаем Unicode текст
	QString htmlText;

	// Для HTML сначала пробуем определить кодировку через QTextCodec::codecForHtml
	// (он умеет искать meta charset и XML declaration)
	QTextCodec* codec = QTextCodec::codecForHtml(rawData);
	if (codec) {
		htmlText = codec->toUnicode(rawData);
	}
	else {
		// Если не получилось, используем нашу библиотеку
		AutoIt::Common::TextEncodingDetect detector;
		AutoIt::Common::TextEncodingDetect::Encoding encoding = detector.DetectEncoding(
			reinterpret_cast<const unsigned char*>(rawData.constData()),
			rawData.size()
		);
		htmlText = decodeWithEncoding(rawData, encoding);
	}

	// В зависимости от режима возвращаем либо полный HTML, либо чистый текст
	if (mode == ExtractTextOnly) {
		return extractPlainTextFromHtml(htmlText);
	}

	return htmlText;
}

QString FileExtractor::extractPlainTextFromHtml(const QString& html)
{
	QString plainText;
	plainText.reserve(html.size());

	bool inTag = false;
	bool inHead = false;
	bool inScript = false;
	bool inStyle = false;

	bool justClosedTag = false;
	bool lastWasSpace = false;

	// Список тегов, после которых нужно добавить перенос строки
	QStringList blockTags = { "p", "div", "h1", "h2", "h3", "h4", "h5", "h6",
							 "br", "hr", "table", "tr", "li", "section", "article" };
	

	for (int i = 0; i < html.size(); ++i) {
		QChar c = html[i];

		if (c == '<') {
			inTag = true;

			// Сохраняем имя тега для дальнейшей обработки
			QString tagName;
			int j = i + 1;
			while (j < html.size() && html[j].isLetter()) {
				tagName.append(html[j]);
				j++;
			}
			tagName = tagName.toLower();

			// Проверяем, не начало ли это специального блока
			if (tagName == "head") {
				inHead = true;
			}
			else if (tagName == "script") {
				inScript = true;
			}
			else if (tagName == "style") {
				inStyle = true;
			}

			// Для закрывающих тегов
			if (i + 1 < html.size() && html[i + 1] == '/') {
				j = i + 2;
				tagName.clear();
				while (j < html.size() && html[j].isLetter()) {
					tagName.append(html[j]);
					j++;
				}
				tagName = tagName.toLower();

				if (tagName == "head") {
					inHead = false;
				}
				else if (tagName == "script") {
					inScript = false;
				}
				else if (tagName == "style") {
					inStyle = false;
				}
			}

			continue;
		}

		if (c == '>') {
			inTag = false;
			justClosedTag = true;
			continue;
		}

		if (inTag || inHead || inScript || inStyle) {
			continue;
		}

		// Обработка HTML entities (как в предыдущей версии)
		if (c == '&') {
			int semicolonPos = html.indexOf(';', i);
			if (semicolonPos > i) {
				QString entity = html.mid(i + 1, semicolonPos - i - 1);
				if (entity == "nbsp") {
					plainText.append(' ');
					lastWasSpace = true;
					justClosedTag = false;
				} // ... остальные entity
				i = semicolonPos;
				continue;
			}
		}

		// Вставка пробелов между словами
		if (justClosedTag) {
			if (!c.isSpace() && !c.isPunct() && !plainText.isEmpty() && !lastWasSpace) {
				plainText.append(' ');
				lastWasSpace = true;
			}
			justClosedTag = false;
		}

		// Обработка пробелов
		if (c.isSpace()) {
			if (!lastWasSpace && !plainText.isEmpty()) {
				plainText.append(' ');
				lastWasSpace = true;
			}
			while (i + 1 < html.size() && html[i + 1].isSpace()) {
				i++;
			}
			continue;
		}

		plainText.append(c);
		lastWasSpace = false;
	}

	

	// Простая эвристика: ищем точки, двоеточия и увеличиваем расстояние
	plainText.replace(". ", ".\n");
	plainText.replace("! ", "!\n");
	plainText.replace("? ", "?\n");
	plainText.replace(": ", ":\n");

	// Чистим лишние переносы
	plainText.replace(QRegularExpression("\n{3,}"), "\n\n");

	return plainText.trimmed();
}

QString FileExtractor::handlePlainText(const QByteArray& rawData)
{
	AutoIt::Common::TextEncodingDetect detector;
	AutoIt::Common::TextEncodingDetect::Encoding encoding = detector.DetectEncoding(
		reinterpret_cast<const unsigned char*>(rawData.constData()),
		rawData.size()
	);

	QString text = decodeWithEncoding(rawData, encoding);
	// Нормализуем концы строк к единому формату (\n)
	text.replace("\r\n", "\n"); // Windows -> Unix
	text.replace("\r", "\n");   // Mac Classic -> Unix

	return text;
}

QString FileExtractor::decodeWithEncoding(const QByteArray& data, AutoIt::Common::TextEncodingDetect::Encoding encoding)
{
	switch (encoding) {
	case AutoIt::Common::TextEncodingDetect::ASCII:
	case AutoIt::Common::TextEncodingDetect::ANSI:
		// Для ANSI/ASCII используем локальную кодировку системы
		// В Windows это обычно Windows-1251 или другая
		return QString::fromLocal8Bit(data);

	case AutoIt::Common::TextEncodingDetect::UTF8_BOM:
	case AutoIt::Common::TextEncodingDetect::UTF8_NOBOM:
		return QString::fromUtf8(data);

	case AutoIt::Common::TextEncodingDetect::UTF16_LE_BOM:
	case AutoIt::Common::TextEncodingDetect::UTF16_LE_NOBOM:
		return QString::fromUtf16(reinterpret_cast<const char16_t*>(data.constData()),
			data.size() / 2);

	case AutoIt::Common::TextEncodingDetect::UTF16_BE_BOM:
	case AutoIt::Common::TextEncodingDetect::UTF16_BE_NOBOM: {
		// Для UTF-16 BE нужно преобразовать в LE или использовать QTextCodec
		QTextCodec* codec = QTextCodec::codecForName("UTF-16BE");
		if (codec) {
			return codec->toUnicode(data);
		}
		// Fallback: конвертируем вручную
		QByteArray leData = data;
		char* rawData = leData.data();
		for (int i = 0; i < leData.size(); i += 2) {
			if (i + 1 < leData.size()) {
				// Меняем местами байты
				char tmp = rawData[i];
				rawData[i] = rawData[i + 1];
				rawData[i + 1] = tmp;
			}
		}
		return QString::fromUtf16(reinterpret_cast<const char16_t*>(leData.constData()),
			leData.size() / 2);
	}

	case AutoIt::Common::TextEncodingDetect::None:
	default:
		qWarning() << "Unknown encoding, trying local 8-bit";
		return QString::fromLocal8Bit(data);
	}
}


// Обновляем старый метод для обратной совместимости
QString FileExtractor::decodeQuotedPrintable(const QByteArray& data)
{
	return QString::fromLatin1(decodeQuotedPrintableToBytes(data));
}


QByteArray FileExtractor::extractHtmlFromMhtml(const QByteArray& mhtmlData)
{
	// Простой парсер MHTML формата
	// Ищем границы частей (boundary) и Content-Type: text/html

	QString data = QString::fromLatin1(mhtmlData); // MHTML заголовки всегда в ASCII

	// Ищем boundary из заголовка Content-Type
	QRegularExpression boundaryRegex("boundary=\"?([^\";\\s]+)\"?");
	QRegularExpressionMatch boundaryMatch = boundaryRegex.match(data);

	if (!boundaryMatch.hasMatch()) {
		return QByteArray();
	}

	QString boundary = "--" + boundaryMatch.captured(1);
	QStringList parts = data.split(boundary);

	for (const QString& part : parts) {
		if (part.contains("Content-Type: text/html", Qt::CaseInsensitive)) {
			// Нашли HTML часть
			QStringList lines = part.split('\n');
			bool contentStarted = false;
			QByteArray html;

			for (const QString& line : lines) {
				if (!contentStarted) {
					// Пропускаем заголовки до первой пустой строки
					if (line.trimmed().isEmpty()) {
						contentStarted = true;
					}
				}
				else {
					// Сохраняем как Latin1, потому что quoted-printable работает на байтах
					html.append(line.toLatin1());
					html.append('\n');
				}
			}

			return html;
		}
	}

	return QByteArray();
}
