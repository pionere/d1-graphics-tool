#include "d1cpp.h"

#include <stack>

#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>

#include "config.h"
#include "mainwindow.h"
#include "progressdialog.h"

#include "dungeon/all.h"

typedef enum Read_State {
    READ_BASE,
    READ_QUOTE_SINGLE,
    READ_QUOTE_DOUBLE,
    READ_COMMENT_SINGLE,
    READ_COMMENT_MULTI,
    READ_DIRECTIVE,
    READ_NUMBER,
    READ_TABLE,
    READ_ROW_SIMPLE,
    READ_ROW_COMPLEX,
    READ_ROW_COMPLEX_POST,
    READ_ENTRY_SIMPLE,
    READ_ENTRY_COMPLEX,
} Read_State;

typedef enum LogLevel {
    LOG_ERROR,
    LOG_WARN,
    LOG_NOTE,
} LogLevel;

#define LOG_LEVEL LOG_NOTE
/*#define LogMessage(msg, lvl) \
if (lvl <= LOG_LEVEL) { \
    if (lvl == LOG_ERROR) dProgressErr() << msg; \
    else if (lvl == LOG_WARN) dProgressWarn() << msg; \
    else dProgress() << msg; \
}*/
#define LogMessage(msg, lvl) LogErrorF((msg).toLatin1().constData())

static QString newLine;
static D1CppTable *currTable = nullptr;
static D1CppRow *currRow = nullptr;
static D1CppRowEntry *currRowEntry = nullptr;
static D1CppEntryData *currEntryData = nullptr;
static std::pair<int, QString> currState;
static std::stack<std::pair<int, QString>> states;
static void cleanup()
{
    MemFree(currTable);
    MemFree(currRow);
    MemFree(currRowEntry);
    MemFree(currEntryData);
    currState.second.clear();
    states = std::stack<std::pair<int, QString>>();
}

bool D1Cpp::processContent(int type)
{
    QString content = currState.second;
    currState = states.top();
    states.pop();

    switch (currState.first) {
    case READ_BASE:
        switch (type) {
        case READ_QUOTE_SINGLE:
            content.prepend('\'');
            content.append('\'');
            break;
        case READ_QUOTE_DOUBLE:
            content.prepend('"');
            content.append('"');
            break;
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            content.append(newLine);
            break;
        case READ_COMMENT_MULTI:
            content.prepend("/*");
            content.append("*/");
            break;
        // case READ_DIRECTIVE:
        // case READ_NUMBER:
        case READ_TABLE:
            LogMessage(QString("Table %1 done.").arg(currTable->name), LOG_NOTE);
            this->tables.push_back(currTable);
            currTable = nullptr;
            return true;
        // case READ_ROW_SIMPLE:
        // case READ_ROW_COMPLEX:
        // case READ_ENTRY_SIMPLE:
        // case READ_ENTRY_COMPLEX:
        default:
            LogMessage(QString("Invalid type (%1) when reading base content.").arg(type), LOG_ERROR);
            return false;
        }
        LogMessage(QString((type == READ_QUOTE_SINGLE || type == READ_QUOTE_DOUBLE) ? "Skipping quote %1." : "Skipping comment %1.").arg(content), LOG_NOTE);
        currState.second.append(content);
        break;
    // case READ_QUOTE_SINGLE:
    // case READ_QUOTE_DOUBLE:
    // case READ_COMMENT_SINGLE:
    // case READ_COMMENT_MULTI:
    // case READ_DIRECTIVE:
    // case READ_NUMBER:
    case READ_TABLE:
        switch (type) {
        // case READ_QUOTE_SINGLE:
        // case READ_QUOTE_DOUBLE:
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            content.append(newLine);
            break;
        case READ_COMMENT_MULTI:
            content.prepend("/*");
            content.append("*/");
            break;
        case READ_DIRECTIVE:
            content.prepend("#");
            content.append(newLine);
            break;
        // case READ_NUMBER:
        // case READ_TABLE:
        case READ_ROW_SIMPLE:
        case READ_ROW_COMPLEX:
            LogMessage(QString("Row %1 (%3) of %2 done.").arg(currTable->rows.size()).arg(currTable->name).arg(type == READ_ROW_COMPLEX), LOG_NOTE);

            currTable->rows.push_back(currRow);
            currTable->rowTexts.push_back(QString());
            currRow = nullptr;
            return true;
        // case READ_ENTRY_SIMPLE:
        // case READ_ENTRY_COMPLEX:
        default:
            LogMessage(QString("Invalid type (%1) when reading table content.").arg(type), LOG_ERROR);
            return false;
        }
        currTable->rowTexts[currTable->rows.count()].append(content);
        LogMessage(QString("Table comment %1.").arg(content), LOG_NOTE);
        break;
    case READ_ROW_SIMPLE:
    case READ_ROW_COMPLEX:
        switch (type) {
        // case READ_QUOTE_SINGLE:
        // case READ_QUOTE_DOUBLE:
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            content.append(newLine);
            break;
        case READ_COMMENT_MULTI:
            content.prepend("/*");
            content.append("*/");
            break;
        // case READ_DIRECTIVE:
        // case READ_NUMBER:
        // case READ_TABLE:
        // case READ_ROW_SIMPLE:
        // case READ_ROW_COMPLEX:
        case READ_ENTRY_SIMPLE:
            currEntryData->content.append(content.trimmed());

            content.clear();

            currRowEntry->datas.push_back(currEntryData);
            currRowEntry->dataTexts.push_back(QString());
            currEntryData = nullptr;
            // fallthrough
        case READ_ENTRY_COMPLEX:
            LogMessage(QString("Entry %1 (%5) of row %2 of table %3 done with content %4.").arg(currRow->entries.size()).arg(currTable->rows.size()).arg(currTable->name).arg(content.trimmed()).arg(type == READ_ENTRY_COMPLEX), LOG_NOTE);

            currRowEntry->dataTexts.back().append(content.trimmed());
            currRow->entries.push_back(currRowEntry);
            currRow->entryTexts.push_back(QString());
            currRowEntry = nullptr;
            LogMessage(QString("Entry added."), LOG_NOTE);
            // if (currState.first == READ_ROW_SIMPLE) {
            //    processContent(READ_ROW_SIMPLE);
            // }
            return true;
        default:
            LogMessage(QString("Invalid type (%1) when reading row content.").arg(type), LOG_ERROR);
            return false;
        }
        currRow->entryTexts[currRow->entries.count()].append(content);
        LogMessage(QString("Row comment %1.").arg(content), LOG_NOTE);
        break;
    case READ_ENTRY_SIMPLE:
    case READ_ENTRY_COMPLEX:
        switch (type) {
        // case READ_QUOTE_SINGLE:
        // case READ_QUOTE_DOUBLE:
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            content.append(newLine);
            break;
        case READ_COMMENT_MULTI:
            content.prepend("/*");
            content.append("*/");
            break;
        // case READ_DIRECTIVE:
        // case READ_NUMBER:
        // case READ_TABLE:
        // case READ_ROW_SIMPLE:
        // case READ_ROW_COMPLEX:
        // case READ_ENTRY_SIMPLE:
        case READ_ENTRY_COMPLEX:
            LogMessage(QString("Complex content %4 of entry %1 of row %2 of table %3.").arg(currRow->entries.size()).arg(currTable->rows.size()).arg(currTable->name).arg(content), LOG_NOTE);
            if (currState.first == READ_ENTRY_COMPLEX) {
                /*if (!currRowEntry->postContent.isEmpty()) {
                    currRowEntry->content.append(currRowEntry->postContent);
                    currRowEntry->postContent.clear();
                }
                currRowEntry->content.append(content.trimmed());*/
                currEntryData->content.append(content.trimmed());
                currRowEntry->datas.push_back(currEntryData);
                currRowEntry->dataTexts.push_back(QString());
                currEntryData = nullptr;
                LogMessage(QString("Entry added to a complex entry."), LOG_NOTE);
                return true;
            }
            // fallthrough
        default:
            LogMessage(QString("Invalid type (%1) when reading entry content.").arg(type), LOG_ERROR);
            return false;
        }
        if (currEntryData->content.isEmpty()) {
            currEntryData->preContent.append(content);
        } else {
            currEntryData->postContent.append(content);
        }
        LogMessage(QString("Entry comment %1.").arg(content), LOG_NOTE);
        break;
    }
    return true;
}

typedef enum Read_Table_State {
    READ_TABLE_BASE,
    READ_TABLE_BRACKET_END,
    READ_TABLE_EXPRESSION,
    // READ_TABLE_BRACKET_BEGIN,
    READ_TABLE_NAME,
    READ_TABLE_TYPE,
    READ_TABLE_PREFIX,
} Read_Table_State;
bool D1Cpp::initTable()
{
    const QString &content = currState.second;

    LogMessage(QString("Checking for table: \n... %1 ...\n").arg(content), LOG_NOTE);

    int idx = content.length() - 1;
    int tableState = READ_TABLE_BASE;
    QString tableName;
    while (true) {
        if (idx < 0 || content[idx] == '\\') {
            LogMessage(QString("Not a table 0."), LOG_NOTE);
            return false;
        }
        // if (content[idx].isSpace()) {
        if (content[idx] == ' ') {
            idx--;
            continue;
        }
        switch (tableState) {
        case READ_TABLE_BASE:
            if (content[idx] == '=') {
                idx--;
                tableState = READ_TABLE_BRACKET_END;
                continue;
            }
            LogMessage(QString("Not a table 1."), LOG_NOTE);
            return false;
        case READ_TABLE_BRACKET_END:
            if (content[idx] == ']') {
                idx--;
                tableState = READ_TABLE_EXPRESSION;
                continue;
            }
            LogMessage(QString("Not a table 2."), LOG_NOTE);
            return false;
        case READ_TABLE_EXPRESSION:
            if (content[idx] == '[') {
                tableState = READ_TABLE_NAME;
            }
            idx--;
            continue;
        case READ_TABLE_NAME: {
            int lastIdx = idx;
            // while (!content[idx].isSpace()) {
            while (content[idx] != ' ') {
                idx--;
                if (idx < 0) {
                    LogMessage(QString("Not a table 3."), LOG_NOTE);
                    return false;
                }
            }
            tableName = content.mid(idx + 1, lastIdx - idx);
            tableState = READ_TABLE_TYPE;
        } continue;
        case READ_TABLE_TYPE:
            while (content[idx] != ' ') {
                idx--;
                if (idx < 0) {
                    LogMessage(QString("Not a table 4."), LOG_NOTE);
                    return false;
                }
            }
            tableState = READ_TABLE_PREFIX;
            continue;
        case READ_TABLE_PREFIX:
            static char *prefixes[] = { "const", "static", "constexpr" };
            bool prefixFound = false;
            for (int i = 0; i < lengthof(prefixes); ) {
                int len = strlen(prefixes[i]);
                if (idx >= len - 1 && content.mid(idx - (len - 1), len) == prefixes[i]) {
                    prefixFound = true;
                    idx -= len;
                    while (idx >= 0 && content[idx] == ' ') {
                        idx--;
                    }
                    if (idx < 0) {
                        break;
                    }
                    i = 0;
                } else {
                    i++;
                }
            }
            if (!prefixFound) {
                LogMessage(QString("Not a table 5 %1.").arg(content.mid(idx)), LOG_NOTE);
                return false;
            }
            if (idx >= 0) {
                int len = newLine.length();
                QString conLine = content.mid(idx - (len - 1), len);
                if (conLine != newLine) {
                    LogMessage(QString("Not a table 6 %1.").arg(conLine), LOG_NOTE);
                    return false;
                }
            }
            break;
        }
        break;
    }
    LogMessage(QString("Found table: %1.").arg(tableName), LOG_NOTE);

    currTable = new D1CppTable(tableName);
    currTable->rowTexts.push_back(QString());
    return true;
}

void D1Cpp::initRow() 
{
    currRow = new D1CppRow();
    currRow->entryTexts.push_back(QString());
}

void D1Cpp::initRowEntry() 
{
    currRowEntry = new D1CppRowEntry();
    currRowEntry->dataTexts.push_back(QString());
}

bool D1Cpp::readContent(QString &content)
{
    while (!content.isEmpty()) {
        switch (currState.first) {
        case READ_BASE:
            if (content[0] == '"' || content[0] == '\'') {
                bool single = content[0] == '\'';
                content.remove(0, 1);
                states.push(currState);
                currState.first = single ? READ_QUOTE_SINGLE : READ_QUOTE_DOUBLE;
                currState.second = "";
                LogMessage(QString("Starting quote %1.").arg(content), LOG_NOTE);
                continue;
            }
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(currState);
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second = "";
                    LogMessage(QString("Starting comment %1.").arg(content), LOG_NOTE);
                    continue;
                }
            }
            if (content[0] == '{') {
                content.remove(0, 1);
                if (initTable()) {
                    this->texts.back().append(currState.second);
                    this->texts.push_back(QString());
                    currState.second.clear();

                    states.push(currState);
                    currState.first = READ_TABLE;
                    currState.second = "";
                    LogMessage(QString("Starting table %1.").arg(content), LOG_NOTE);
                } else {
                    currState.second.append('{');
                }
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }

            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_QUOTE_SINGLE:
            if (content[0] == '\'') {
                content.remove(0, 1);
                if (!processContent(READ_QUOTE_SINGLE)) {
                    return false;
                }
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }
            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_QUOTE_DOUBLE:
            if (content[0] == '"') {
                content.remove(0, 1);
                if (!processContent(READ_QUOTE_DOUBLE)) {
                    return false;
                }
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }
            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_COMMENT_SINGLE:
        case READ_DIRECTIVE:
            if (content.startsWith(newLine)) {
                content.remove(0, newLine.length());
                if (!processContent(currState.first)) {
                    return false;
                }
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }
            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_COMMENT_MULTI:
            if (content[0] == '*') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/') {
                    content.remove(0, 2);
                    if (!processContent(READ_COMMENT_MULTI)) {
                        return false;
                    }
                    continue;
                }
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }
            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        // case READ_NUMBER:
        case READ_TABLE:
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(currState);
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second = "";
                    continue;
                }
            }
            if (content[0] == '#') {
                content.remove(0, 1);
                states.push(currState);
                currState.first = READ_DIRECTIVE;
                currState.second = "";
                continue;
            }
            if (content[0] == '}') {
                content.remove(0, 1);
                if (!processContent(READ_TABLE)) {
                    return false;
                }
                continue;
            }
            if (content[0] == '{') {
				content.remove(0, 1); // TODO: or complex entry?
                LogMessage(QString("Starting complex row %1.").arg(content), LOG_NOTE);
                initRow();
                states.push(currState);
                currState.first = READ_ROW_COMPLEX;
                currState.second = "";
                continue;
            }
            if (!content[0].isSpace()) {
                LogMessage(QString("Starting simple row %1.").arg(content), LOG_NOTE);
                initRow();
                states.push(currState);
                currState.first = READ_ROW_SIMPLE;
                currState.second = "";
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }

            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_ROW_SIMPLE:
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(currState);
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second = "";
                    continue;
                }
            }
            if (content[0] == '}') {
                if (!processContent(READ_ROW_SIMPLE)) {
                    return false;
                }
                continue;
            }
            if (content[0] == '{') {
                content.remove(0, 1);
                states.push(currState);
                currState.first = READ_ENTRY_COMPLEX;
                currState.second = "";
                continue;
            }
            if (!content[0].isSpace()) {
                initRowEntry();
                currEntryData = new D1CppEntryData(); // initEntryData

                states.push(currState);
                currState.first = READ_ENTRY_SIMPLE;
                currState.second = "";
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }

            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_ROW_COMPLEX:
            // LogMessage(QString("Processing complex row %1.").arg(content), LOG_NOTE);
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(currState);
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second = "";
                    continue;
                }
            }
            if (content[0] == '}') {
                content.remove(0, 1);
                /*if (!processContent(READ_ROW_COMPLEX)) {
                    return false;
                }*/
                // states.push(currState);
                currState.first = READ_ROW_COMPLEX_POST;
                continue;
            }
            if (content[0] == '{') {
LogMessage(QString("Starting complex entry in a complex row %1.").arg(content), LOG_NOTE);
                content.remove(0, 1);
                initRowEntry();

                states.push(currState);
                currState.first = READ_ENTRY_COMPLEX;
                currState.second = "";
                continue;
            }
            if (!content[0].isSpace()) {
LogMessage(QString("Starting simple entry in a complex row %1.").arg(content), LOG_NOTE);
                initRowEntry();
                currEntryData = new D1CppEntryData(); // initEntryData

                states.push(currState);
                currState.first = READ_ENTRY_SIMPLE;
                currState.second = "";
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }

            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_ROW_COMPLEX_POST:
            // LogMessage(QString("Processing complex row %1.").arg(content), LOG_NOTE);
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(currState);
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second = "";
                    continue;
                }
            }
            if (content[0] == ',') {
                content.remove(0, 1);
                if (!processContent(READ_ROW_COMPLEX)) { // READ_ROW_COMPLEX_POST?
                    return false;
                }
                continue;
            }
            if (content[0] == '}') {
                if (!processContent(READ_ROW_COMPLEX)) { // READ_ROW_COMPLEX_POST?
                    return false;
                }
                continue;
            }
            if (!content[0].isSpace()) {
                return false;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }

            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_ENTRY_SIMPLE:
            /*if (content[0].isSpace()) {
                content.remove(0, 1);
                continue;
            }*/
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(currState);
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second = "";
                    continue;
                }
            }
            if (content[0] == ',') {
                content.remove(0, 1);
                if (!processContent(READ_ENTRY_SIMPLE)) {
                    return false;
                }
                continue;
            }
            if (content[0] == '}') {
                if (!processContent(READ_ENTRY_SIMPLE)) {
                    return false;
                }
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }

            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        case READ_ENTRY_COMPLEX:
            /*if (content[0].isSpace()) {
                content.remove(0, 1);
                continue;
            }*/
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(currState);
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second = "";
                    continue;
                }
            }
            /*if (content[0] == '{') {
                content.remove(0, 1);

                states.push(currState);
                currState.first = READ_ENTRY_COMPLEX;
                currState.second = "";
                continue;
            }
            if (content[0] == ',') {
                content.remove(0, 1);

                if (!processContent(READ_ENTRY_COMPLEX)) {
                    return false;
                }
                continue;
            }*/
            if (content[0] == '}') {
                if (!processContent(READ_ENTRY_COMPLEX)) {
                    return false;
                }
                continue;
            }
            /*if (content[0] == '{') {
                content.remove(0, 1);
                initRowEntry();

                states.push(currState);
                currState.first = READ_ENTRY_COMPLEX;
                currState.second = "";
                continue;
            }*/
            if (!content[0].isSpace()) {
LogMessage(QString("Starting simple entry in a complex entry %1.").arg(content), LOG_NOTE);
                currEntryData = new D1CppEntryData(); // initEntryData

                states.push(currState);
                currState.first = READ_ENTRY_SIMPLE;
                currState.second = "";
                continue;
            }
            if (content[0] == '\\') {
                if (content.length() < 2) {
                    return true;
                }
                currState.second.append(content[0]);
                content.remove(0, 1);
            }

            currState.second.append(content[0]);
            content.remove(0, 1);
            continue;
        }
    }
    return true;
}

QString D1CppEntryData::getContent() const
{
    return this->content;
}

void D1CppEntryData::setContent(const QString &text)
{
    this->content = text;
}

QString D1CppRowEntry::getContent() const
{
    return this->datas.empty() ? "" : this->datas[0]->getContent();
}

void D1CppRowEntry::setContent(const QString &text)
{
    this->datas[0]->setContent(text);
}

D1CppRow::~D1CppRow()
{
    qDeleteAll(this->entries);
    this->entries.clear();
}

D1CppRowEntry *D1CppRow::getEntry(int index) const
{
    return const_cast<D1CppRowEntry *>(this->entries[index]);
}

QString D1CppRow::getEntryText(int index) const
{
    return this->entryTexts[index];
}

D1CppTable::D1CppTable(const QString &n)
    : name(n)
{
}

D1CppTable::~D1CppTable()
{
    qDeleteAll(this->rows);
    this->rows.clear();
}

QString D1CppTable::getName() const
{
    return this->name;
}

int D1CppTable::getRowCount() const
{
    return this->rows.count();
}

int D1CppTable::getColumnCount() const
{
    return this->rows.isEmpty() ? 0 : this->rows[0]->entries.count();
}

D1CppRow *D1CppTable::getRow(int index) const
{
    return const_cast<D1CppRow *>(this->rows[index]);
}

QString D1CppTable::getRowText(int index) const
{
    return this->rowTexts[index];
}

QString D1CppTable::getHeader(int index) const
{
    return this->header[index];
}

QString D1CppTable::getLeader(int index) const
{
    return this->leader[index];
}

D1Cpp::~D1Cpp()
{
    qDeleteAll(this->tables);
    this->tables.clear();
}

bool D1Cpp::postProcess()
{
    bool change = false;
    for (D1CppTable *table : this->tables) {
        // balance the table
        LogMessage(QString("Found table %1: %2 x %3.").arg(table->getName()).arg(table->getRowCount()).arg(table->getColumnCount()), LOG_NOTE);
        int columnNum = 0;
        bool ch = false;
        for (int i = 0; i < table->getRowCount(); i++) {
            D1CppRow *row = table->getRow(i);
            int cc = row->entries.count();
            if (i == 0 || columnNum < cc) {
                columnNum = cc;
                ch = i != 0;
            }
        }
        if (ch) {
            change = true;
            dProgressWarn() << tr("Entries added to unbalanced table %1").arg(table->getName());
            for (int i = 0; i < table->getRowCount(); i++) {
                D1CppRow *row = table->getRow(i);
                while (row->entries.count() < columnNum) {
                    row->entries.push_back(new D1CppRowEntry());
                    row->entryTexts.push_back(QString());
                }
            }
        }
        // add leader fields
        for (int i = 0; i < table->getRowCount(); i++) {
            QString &firstText = table->rowTexts[i];
            QString rowLeader;
            if (firstText.endsWith("*/")) {
                int x = firstText.length() - (1 + 2);
                for (; x > 0; x--) {
                    if (firstText[x] == '*' && firstText[x - 1] == '/') {
                        break;
                    }
                    // TODO: check lineEnd?
                }
                if (x > 0) {
                    rowLeader = firstText.mid(x + 1, firstText.length() - (1 + 2 + x + 1) + 1);
if (i == 0)
        LogMessage(QString("Adding leader field %1 to row %2 rem:%3.").arg(rowLeader).arg(i).arg(x), LOG_NOTE);
                    firstText = firstText.left(x - 1);
                }

            }
            table->leader.push_back(rowLeader);
        }

        // add header fields
        if (table->getColumnCount() != 0) {
            QString &firstText = table->rowTexts[0];
        LogMessage(QString("Checking for header info (len %2) in %1.").arg(firstText).arg(firstText.length()), LOG_NOTE);
            if (firstText.endsWith(this->lineEnd)) {
        LogMessage(QString("LineEnd found."), LOG_NOTE);
                int x = firstText.length() - (1 + this->lineEnd.length());
                for ( ; x > 0; x--) {
                    if (firstText[x] == '/' && firstText[x - 1] == '/') {
                        break;
                    }
                    // TODO: check lineEnd?
                }
                if (x > 0) {
        LogMessage(QString("Found header info at %1.").arg(x), LOG_NOTE);
                    QString headerText = firstText.mid(x + 1, firstText.length() - (this->lineEnd.length() + x + 1));
                    headerText = headerText.trimmed();
                    QStringList headerNames = headerText.split(',');
                    if (headerNames.count() > 1) { // TODO: check against ColumnCount
        LogMessage(QString("Found header names %1.").arg(headerNames.count()), LOG_NOTE);

                        firstText.truncate(x - 1);
                        for (QString &headerName : headerNames) {
                            table->header.push_back(headerName.trimmed());
                        }
                        if (table->header[0].isEmpty()) {
                            table->header[0] = " ";
                        }
                    }
                }
        LogMessage(QString("Resulting firstText %1.").arg(table->rowTexts[0]), LOG_NOTE);
            }
        LogMessage(QString("Header count %1 columns %2.").arg(table->header.count()).arg(table->getColumnCount()), LOG_NOTE);
            while (table->header.count() < table->getColumnCount()) {
                table->header.push_back(QString());
            }
        }
    }
    LogMessage(QString("postProcess done."), LOG_NOTE);

    return change;
}

bool D1Cpp::load(const QString &filePath)
{
    // prepare file data source
    QFile file;
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    QTextStream txt(&file);
    newLine = "\n";
    while (!txt.atEnd()) {
        QChar chr;
        txt >> chr;
        if (chr == '\r') {
            txt >> chr;
            newLine = chr == '\n' ? "\r\n" : "\r";
            break;
        } else if (chr == '\n') {
            // newLine = "\n";
            break;
        }
    }
    txt.seek(0);

    // int readState = READ_CONTENT;
    currState.first = READ_BASE;
    currState.second = "";

    QString content = "";
    this->texts.push_back(QString());
    while (!txt.atEnd()) {
        content.append(txt.read(1024));
        if (!readContent(content)) {
            cleanup();
            // qDeleteAll(this->tables);
            // this->tables.clear();
            return false;
        }
    }
    if (currState.first != READ_BASE || currState.second.isEmpty() || currTable != nullptr || currRow != nullptr || currRowEntry != nullptr || currEntryData != nullptr) {
        cleanup();
        // qDeleteAll(this->tables);
        // this->tables.clear();
        return false;
    }

    this->texts.back().append(currState.second);
    this->lineEnd = newLine;

    bool change = this->postProcess();

    this->cppFilePath = filePath;
    this->modified = change;
    return true;
}

bool D1Cpp::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    QString targetFilePath = params.celFilePath;
    if (!targetFilePath.isEmpty()) {
        filePath = targetFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    } else if (!this->isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    QTextStream out(&outFile);

    int i = 0;
    for ( ; i < this->tables.count(); i++) {
        out << this->texts[i];

        out << "{\n";

        D1CppTable *table = this->tables[i];
        int maxLen = 0;
        for (const QString &leader : table->leader) {
            int len = leader.length();
            if (len > maxLen) {
                maxLen = len;
            }
        }
        QList<int> maxWidths;
        maxWidths.push_back(maxLen);
        QList<QList<QString>> entryContents;
        for (const QString &header : table->header) {
            maxWidths.push_back(header.length());
        }
        for (int n = 0; n < table->rows.count(); n++) {
            QList<QString> rowEntryContents;
            const D1CppRow *row = table->rows[n];
            int w = 0;
            for (D1CppRowEntry *entry : row->entries) {
                // TODO: handle row->entryTexts[e]
                for (D1CppEntryData *data : entry->datas) {
                    w++;

                    QString dataContent;
                    /*if (w == 1 && entry->isComplex) {
                        dataContent = "{ ";
                    }*/
                    dataContent = data->preContent + data->content + data->postContent;
                    int len = dataContent.length();
                    if (maxWidths[w] < len) {
                        maxWidths[w] = len;
                    }
                    rowEntryContents.push_back(dataContent);
                }
                /*if (entry->isComplex) {
                    rowEntryContents.back().append(" }");
                }*/
            }
            entryContents.push_back(rowEntryContents);
        }
        int n = 0;
        for ( ; n < table->rows.count(); n++) {
            out << table->rowTexts[n];

            // add header
            if (n == 0 && !table->header[0].isEmpty()) {
                QString content = "//";
                content = content.leftJustified(maxWidths[0] + 4 + 1 + 2, ' '); // 2 if row->isComplex
                out << content;
                for (int h = 0; ; h++) {
                    QString content = table->header[h];
                    bool last = h == table->header.count() - 1;
                    if (!last) {
                        content += ", ";
                        content = content.leftJustified(maxWidths[h + 1] + 2, ' ');
                    }
                    out << content;
                    if (last) {
                        break;
                    }
                }
                out << "\n";
            }
            // add leader
            if (maxWidths[0] != 0) {
                QString content = table->leader[n];
                if (!content.isEmpty()) {
                    content.prepend("/*");
                    content.append("*/ ");
                }
                content = content.leftJustified(maxWidths[0] + 4 + 1, ' ');
                out << content;
            }
            out << "{ "; // if row->isComplex
            int e = 0;
            for ( ; ; e++) {
                /*out << row->entryTexts[e];
                out << row->entries[e]->preContent;
                out << row->entries[e]->content;
                out << row->entries[e]->postContent;*/
                QString content = entryContents[n][e];
                bool last = e == entryContents[n].count() - 1;
                if (!last) {
                    content += ", ";
                }
                content = content.leftJustified(maxWidths[e + 1] + (last ? 0 : 2), ' ');
                out << content;
                if (last) {
                    break;
                }
            }
            out << " }"; // if row->isComplex
            out << ",\n";
            out << table->rows[n]->entryTexts[e];
        }
        out << table->rowTexts[n];
        out << "}";
    }
    out << this->texts[i];

    this->cppFilePath = filePath;
    this->modified = false;
    return true;
}

bool D1Cpp::isModified() const
{
    return this->modified;
}

void D1Cpp::setModified()
{
    this->modified = true;
}

QString D1Cpp::getFilePath() const
{
    return this->cppFilePath;
}

void D1Cpp::setFilePath(const QString &path)
{
    this->cppFilePath = path;
}

int D1Cpp::getTableCount() const
{
    return this->tables.count();
}

D1CppTable *D1Cpp::getTable(int index) const
{
    return const_cast<D1CppTable *>(this->tables[index]);
}
