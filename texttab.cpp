#include "texttab.h"
#include <QVBoxLayout>
#include <QToolBar>
#include <QLabel>
#include <QAction>
#include <QTextCursor>
#include <QTextEdit>
#include <QFileInfo>
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QDebug>

TextTab::TextTab(const QString& filePath,
	const QString& text,
	const QVector<SearchMatch>& matches,
	QWidget* parent)
	: QWidget(parent)
	, m_filePath(filePath)
	, m_matches(matches)
	, m_currentMatch(0)
	, m_matchLabel(nullptr)
	, m_prevAction(nullptr)
	, m_nextAction(nullptr)
	, m_copyAction(nullptr)
	, m_findAction(nullptr)
{
	setupUi();
	m_textEdit->setPlainText(text);
	// Валидируем совпадения после установки текста
	if (!validateMatches()) {
		return;
	}
	highlightMatches();
}

void TextTab::setupUi()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	// Панель инструментов
	QToolBar* toolbar = new QToolBar;
	toolbar->setIconSize(QSize(16, 16));

	m_prevAction = toolbar->addAction("<-", this, &TextTab::prevMatch);
	m_nextAction = toolbar->addAction("->", this, &TextTab::nextMatch);

	toolbar->addSeparator();

	m_matchLabel = new QLabel;
	toolbar->addWidget(m_matchLabel);

	toolbar->addSeparator();

	m_copyAction = toolbar->addAction("Copy", this, &TextTab::copySelection);
	m_findAction = toolbar->addAction("Find", this, &TextTab::findInText);

	toolbar->addSeparator();
	
	layout->addWidget(toolbar);

	// Текстовый редактор
	m_textEdit = new QPlainTextEdit;
	m_textEdit->setReadOnly(true);
	m_textEdit->setFont(QFont("Consolas", 10));
	
	// ВКЛЮЧАЕМ ПЕРЕНОС СТРОК - это то, что нужно
	m_textEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	// Дополнительные настройки для красивого переноса
	m_textEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere); // перенос по словам
	m_textEdit->setMaximumBlockCount(0); // без ограничений на количество блоков

	// Опционально: включаем перенос для удобства чтения
	// m_textEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	// m_textEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

	layout->addWidget(m_textEdit);

	// Контекстное меню
	m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_textEdit, &QPlainTextEdit::customContextMenuRequested,
		[this](const QPoint& pos) {
		QMenu* menu = m_textEdit->createStandardContextMenu();
		menu->addSeparator();
		menu->addAction("Go to Next Match", this, &TextTab::nextMatch);
		menu->addAction("Go to Previous Match", this, &TextTab::prevMatch);
		menu->exec(m_textEdit->mapToGlobal(pos));
		delete menu;
	});

	updateMatchLabel();

	// Оптимизация для больших файлов
	m_textEdit->setMaximumBlockCount(0); // Нет ограничений на количество блоков

	// Отключаем ненужные функции для ускорения
	m_textEdit->document()->setUndoRedoEnabled(false);

	// Настройки отображения
	m_textEdit->setCenterOnScroll(true);
	m_textEdit->setCursorWidth(2);
}

void TextTab::highlightMatches()
{
	if (!m_matches.isEmpty()) {
		m_currentMatch = 0;
		highlightCurrentMatch();
	}

	updateMatchLabel();
}

void TextTab::highlightCurrentMatch()
{
	if (m_matches.isEmpty() || m_currentMatch < 0 || m_currentMatch >= m_matches.size()) {
		return;
	}

	const auto& match = m_matches[m_currentMatch];

	// Создаем курсор и выделяем текст
	QTextCursor cursor(m_textEdit->document());
	cursor.setPosition(match.startPos);
	cursor.setPosition(match.endPos, QTextCursor::KeepAnchor);

	// Устанавливаем курсор и центрируем
	m_textEdit->setTextCursor(cursor);
	m_textEdit->centerCursor();

	// Обновляем выделения (ExtraSelections могут вызывать проблемы)
	// Попробуем вообще без них, только курсор
	// Если нужно подсветить все совпадения, используем другой механизм

	updateMatchLabel();
}

void TextTab::nextMatch()
{
	if (m_matches.isEmpty()) return;
	m_currentMatch = (m_currentMatch + 1) % m_matches.size();
	highlightCurrentMatch();
}

void TextTab::prevMatch()
{
	if (m_matches.isEmpty()) return;
	m_currentMatch = (m_currentMatch - 1 + m_matches.size()) % m_matches.size();
	highlightCurrentMatch();
}

void TextTab::copySelection()
{
	m_textEdit->copy();
}

void TextTab::findInText()
{
	bool ok;
	QString text = QInputDialog::getText(this, "Find", "Text:", QLineEdit::Normal, "", &ok);
	if (ok && !text.isEmpty()) {
		// Используем встроенный поиск QPlainTextEdit
		bool found = m_textEdit->find(text);
		if (!found) {
			// С начала
			QTextCursor cursor = m_textEdit->textCursor();
			cursor.movePosition(QTextCursor::Start);
			m_textEdit->setTextCursor(cursor);
			m_textEdit->find(text);
		}
	}
}

void TextTab::updateMatchLabel()
{
	if (m_matches.isEmpty()) {
		m_matchLabel->setText("No matches");
		m_prevAction->setEnabled(false);
		m_nextAction->setEnabled(false);
	}
	else {
		m_matchLabel->setText(QString("%1/%2 matches")
			.arg(m_currentMatch + 1)
			.arg(m_matches.size()));
		m_prevAction->setEnabled(true);
		m_nextAction->setEnabled(true);
	}
}

bool TextTab::validateMatches()
{
	QTextDocument* doc = m_textEdit->document();
	int docLength = doc->characterCount() - 1; // минус терминатор

	for (int i = m_matches.size() - 1; i >= 0; --i) {
		const auto& match = m_matches[i];
		if (match.startPos < 0 || match.endPos > docLength || match.startPos >= match.endPos) {
			qWarning() << "Removing invalid match:" << match.startPos << match.endPos
				<< "doc length:" << docLength;
			m_matches.removeAt(i);
		}
	}

	if (m_matches.isEmpty()) {
		m_currentMatch = -1;
		m_matchLabel->setText("No valid matches");
		m_prevAction->setEnabled(false);
		m_nextAction->setEnabled(false);
		return false;
	}

	return true;
}


