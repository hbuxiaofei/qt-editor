#include <QEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QFont>
#include <QByteArray>
#include <QMenu>
#include <QToolTip>
#include <QAction>
#include <QPainter>

#include "notepad.h"
#include "completer.h"

/**************MySyntaxHighlighterEditor******************/
MySyntaxHighlighterEditor::MySyntaxHighlighterEditor(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
}


void MySyntaxHighlighterEditor::readSyntaxHighter(const QString &fileName)
{
    QFile file(fileName);
    if (false == file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Open file " << fileName << "error" << __FUNCTION__;
    }

    QTextStream readFileStream(&file);
    QString keyWord;
    QString readLineStr;
    QStringList lineWordList;
    QString matchReString;
    QRegularExpression re("[ ]+");
    int r, g, b;

    while(!readFileStream.atEnd())
    {
        readLineStr = readFileStream.readLine();
        if (readLineStr.startsWith("$$")) { //comment line
            continue;
        }
        readLineStr.replace("\t", " ");
        lineWordList = readLineStr.split(re);
        if (lineWordList.size() != 4) {
            continue;
        }
        keyWord = lineWordList.at(0);
        matchReString.append(keyWord);
        matchReString.append("|");
        r = lineWordList.at(1).toInt();
        g = lineWordList.at(2).toInt();
        b = lineWordList.at(3).toInt();
        syntaxHightMap.insert(keyWord, QColor(r, g, b));
    }
    matchReString.trimmed();
    matchReExpression.setPattern(matchReString);
    qDebug() << "matchReString:" << matchReString;

}

void MySyntaxHighlighterEditor::highlightBlock(const QString &text)
{
    QTextCharFormat myClassFormat;
    myClassFormat.setFontWeight(QFont::Bold);
    QRegularExpressionMatchIterator i = matchReExpression.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        myClassFormat.setForeground(QBrush(syntaxHightMap.value(match.captured())));
        setFormat(match.capturedStart(), match.capturedLength(), myClassFormat);
    }
}

/**************MyGCodeTextEdit******************/
// public functions
MyGCodeTextEdit::MyGCodeTextEdit(QWidget *parent):QPlainTextEdit(parent)
{
    gCodeHighlighter = new MySyntaxHighlighterEditor(this->document());
    gCodeHighlighter->readSyntaxHighter(QString(":/SynatxHight/C.txt")); //设置语法高亮文件

    // 补全列表设置
    QMap<QString, QColor>::iterator iter;
    for (iter = gCodeHighlighter->syntaxHightMap.begin();
         iter != gCodeHighlighter->syntaxHightMap.end(); ++iter) {
        keyWordsList.append(iter.key());
    }
    qDebug() << "keyWordsList" << keyWordsList;

    keyWordsComplter = new MyCompleter(keyWordsList);
    keyWordsComplter->setWidget(this);
    keyWordsComplter->setCaseSensitivity(Qt::CaseInsensitive);
    keyWordsComplter->setCompletionMode(QCompleter::PopupCompletion);
    keyWordsComplter->setMaxVisibleItems(6);

    lineNumberArea = new LineNumberArea(this);
    lineNumberArea->setVisible(true);

    lineSplitArea = new LineSplitArea(this);
    lineSplitArea->setVisible(true);

    connect(keyWordsComplter, SIGNAL(activated(QString)), this, SLOT(onCompleterActivated(QString)));

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineSplitAreaHeight(int)));

    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(onCurosPosChange()));


    updateLineNumberAreaWidth(0);
    updateLineSplitAreaHeight(0);
    highlightCurrentLine();

}

QString MyGCodeTextEdit::wordUnderCursor() const
{
    // 不断向左移动cursor，并选中字符，并查看选中的单词中是否含有空格——空格作为单词的分隔符
    QTextCursor curTextCursor = textCursor();
    QString selectedString;
    while (curTextCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1)) {
        selectedString = curTextCursor.selectedText();
        if (selectedString.startsWith(QString(" "))
                || selectedString.startsWith(QChar(0x422029))) {
            break;
        }

    }
    if (selectedString.startsWith(QChar(0x422029))) {
        selectedString.replace(0, 1, QChar(' '));
    }
    return selectedString.trimmed();

}

int MyGCodeTextEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 4 + fontMetrics().horizontalAdvance(QLatin1Char('M')) * digits;

    return space;
}

//protected Events
void MyGCodeTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (keyWordsComplter) {
        if (keyWordsComplter->popup()->isVisible()) {
            switch(e->key()) {
                case Qt::Key_Escape:
                case Qt::Key_Enter:
                case Qt::Key_Return:
                case Qt::Key_Tab:
                    e->ignore();
                    return;
                default:
                    break;
            }
        } else {
            if (e->key() == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
                this->insertPlainText("    ");
                e->accept();
                return;
            }
        }

        QPlainTextEdit::keyPressEvent(e);

        switch(e->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Left:
            case Qt::Key_Right:
                return;
            default:
                break;
        }

        completerPrefix = this->wordUnderCursor();

        qDebug() << "completerPrefix: " << completerPrefix;

        if (!completerPrefix.length()) {
            QAbstractItemView *popup = keyWordsComplter->popup();
            popup->hide();
            return;
        }
        // 通过设置QCompleter的前缀，来让Completer寻找关键词
        keyWordsComplter->setCompletionPrefix(completerPrefix);
        curTextCursorRect = cursorRect();

        if (keyWordsComplter->completionCount() > 0) {
            keyWordsComplter->complete(QRect(curTextCursorRect.left(), curTextCursorRect.top(), 60, 20));
            keyWordsComplter->setCurrentRow(0);
        } else {
            QAbstractItemView *popup = keyWordsComplter->popup();
            popup->hide();
        }
    }
}

void MyGCodeTextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();

    int fontWidth = fontMetrics().horizontalAdvance(QLatin1Char('M'));

    int width = lineNumberAreaWidth();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));

    int left = cr.left() + width + fontWidth * 100 + fontWidth / 2;
    lineSplitArea->setGeometry(QRect(left, cr.top(), 1, cr.height()));
}

void MyGCodeTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::gray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(),
                             fontMetrics().height(), Qt::AlignCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }

}

void MyGCodeTextEdit::lineSplitAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineSplitArea);
    painter.fillRect(event->rect(), QColor(Qt::cyan).lighter(160));
}

// public slots
void MyGCodeTextEdit::onCompleterActivated(const QString &completion)
{
    QString completionPrefix = wordUnderCursor();
    QString shouldInertText = completion;

    qDebug() << "comletion: " << completion;

    curTextCursor = textCursor();
    if (!completion.contains(completionPrefix)) {
        // delete the previously typed.
        curTextCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                   completionPrefix.size());
        curTextCursor.clearSelection();
    } else {
        // 补全相应的字符
        shouldInertText = shouldInertText.replace(
            shouldInertText.indexOf(completionPrefix), completionPrefix.size(), "");
    }
    curTextCursor.insertText(shouldInertText);
}

void MyGCodeTextEdit::onCurosPosChange()
{
#if 0
    QString completionPrefix = wordUnderCursor();
    if (completionPrefix == "") {
        keyWordsComplter->setCompletionPrefix("----");
        keyWordsComplter->complete(QRect(curTextCursorRect.left(), curTextCursorRect.top(), 60, 20));
        keyWordsComplter->setCurrentRow(0);
    }
#endif
    highlightCurrentLine();
}

void MyGCodeTextEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void MyGCodeTextEdit::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::gray).lighter(140);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void MyGCodeTextEdit::updateLineNumberArea(const QRect & rect, int dy)
{
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void MyGCodeTextEdit::updateLineSplitAreaHeight(int newBlockCount)
{
    if (newBlockCount)
        lineSplitArea->scroll(0, newBlockCount);
}

/**************MyGCodeEditor******************/
NotePad::NotePad(MyGCodeTextEdit *parent) :
    MyGCodeTextEdit(parent)
{
    // 设置字体
    QFont font("Courier New", 10);
    this->setFont(font);
 }

// 查找
int NotePad::search(QString str, bool backward, bool matchCase, bool regExp)
{
    QTextDocument::FindFlags options;

    if (backward) {
        options = QTextDocument::FindBackward;
    }

    if (matchCase) {
        options |= QTextDocument::FindCaseSensitively;
    }

    QTextCursor cursor = textCursor();

    cursor = regExp ? document()->find(QRegExp(str), cursor, options) :
                      document()->find(str, cursor, options);

    if (cursor.isNull()) {
        return false;
    }

    setTextCursor(cursor);
    return true;
}

// 替换
void NotePad::replace(QString str1, QString str2, bool backward, bool matchCase, bool regExp)
{
    QTextCursor cursor = textCursor();

    if (!cursor.hasSelection()) {
        search(str1, backward, matchCase, regExp);
    } else {
        int pos = textCursor().position() - textCursor().selectedText().length();

        cursor.beginEditBlock();
        cursor.insertText(str2);
        cursor.endEditBlock();

        if (backward) {
            cursor.setPosition(pos, QTextCursor::MoveAnchor);
            setTextCursor(cursor);
        }
    }
}

// 替换所有
void NotePad::replaceAll(QString str1, QString str2, bool matchCase, bool regExp)
{
    QTextCursor cursor = textCursor();

    cursor.setPosition(0, QTextCursor::MoveAnchor);
    setTextCursor(cursor);

    while (search(str1, false, matchCase, regExp)) {
        replace(str1, str2, false, matchCase, regExp);
    }
}

NotePad::~NotePad()
{

}
