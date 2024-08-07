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
    READ_ROW_COMPLEX_POST_COMMENT,
    READ_ENTRY_SIMPLE,
    READ_ENTRY_COMPLEX,
    READ_ENTRY_COMPLEX_POST,
} Read_State;

typedef enum LogLevel {
    LOG_ERROR,
    LOG_WARN,
    LOG_NOTE,
} LogLevel;

#define LOG_LEVEL LOG_ERROR
#define LogMessage(msg, lvl)                        \
if (lvl <= LOG_LEVEL) {                               \
    if (lvl == LOG_ERROR) dProgressErr() << msg;      \
    else if (lvl == LOG_WARN) dProgressWarn() << msg; \
    else dProgress() << msg;                          \
}

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
            // content.append(newLine);
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
        LogMessage(QString(type == READ_DIRECTIVE ? "Table directive %1." : "Table comment %1.").arg(content), LOG_NOTE);
        break;
    case READ_ROW_SIMPLE:
    case READ_ROW_COMPLEX:
        switch (type) {
        // case READ_QUOTE_SINGLE:
        // case READ_QUOTE_DOUBLE:
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            // content.append(newLine);
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
            LogMessage(QString("Simple-Entry %1 (%5) of row %2 of table %3 done with content %4.").arg(currRow->entries.size()).arg(currTable->rows.size()).arg(currTable->name).arg(content).arg(type == READ_ENTRY_COMPLEX), LOG_NOTE);
            currEntryData->content.append(content);

            content.clear();

            currRowEntry->datas.push_back(currEntryData);
            currRowEntry->dataTexts.push_back(QString());
            currEntryData = nullptr;
            // fallthrough
        case READ_ENTRY_COMPLEX:
            if (type == READ_ENTRY_COMPLEX)
            LogMessage(QString("Complex-Entry %1 (%5) of row %2 of table %3 done with content %4.").arg(currRow->entries.size()).arg(currTable->rows.size()).arg(currTable->name).arg(content).arg(type == READ_ENTRY_COMPLEX), LOG_NOTE);

            currRowEntry->dataTexts.back().append(content);
            currRow->entries.push_back(currRowEntry);
            currRow->entryTexts.push_back(QString());
            currRowEntry = nullptr;
            LogMessage(QString("Entry %1. added.").arg(currRow->entries.count()), LOG_NOTE);
            // if (currState.first == READ_ROW_SIMPLE) {
            //    processContent(READ_ROW_SIMPLE);
            // }
            return true;
        default:
            LogMessage(QString("Invalid type (%1) when reading row content.").arg(type), LOG_ERROR);
            return false;
        }
        // currRow->entryTexts[currRow->entries.count()].append(content);
        currRow->entryTexts.back().append(content);
        LogMessage(QString("Row comment %1 at %2.").arg(content).arg(currRow->entryTexts.count()), LOG_NOTE);
        break;
    case READ_ROW_COMPLEX_POST:
        switch (type) {
        // case READ_QUOTE_SINGLE:
        // case READ_QUOTE_DOUBLE:
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            // content.append(newLine);
            break;
        case READ_COMMENT_MULTI:
            content.prepend("/*");
            content.append("*/");
            break;
        default:
            LogMessage(QString("Invalid type (%1) when reading entry content in state %2.").arg(type).arg(currState.first), LOG_ERROR);
            return false;
        }
        currRow->entryTexts.back().append(content);
        LogMessage(QString("Row post-comment %1.").arg(content), LOG_NOTE);
        break;
    case READ_ROW_COMPLEX_POST_COMMENT:
        switch (type) {
        // case READ_QUOTE_SINGLE:
        // case READ_QUOTE_DOUBLE:
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            // content.append(newLine);
            break;
        case READ_COMMENT_MULTI:
            content.prepend("/*");
            content.append("*/");
            break;
        default:
            LogMessage(QString("Invalid type (%1) when reading entry content in state %2.").arg(type).arg(currState.first), LOG_ERROR);
            return false;
        }
        currRow->entryTexts.back().append(content); // TODO: better solution? same as READ_ROW_COMPLEX_POST
        LogMessage(QString("Row post-comment++ %1.").arg(content), LOG_NOTE);
        break;
    case READ_ENTRY_SIMPLE:
    case READ_ENTRY_COMPLEX:
        switch (type) {
        case READ_QUOTE_SINGLE:
            content.prepend('\'');
            content.append('\'');
            currEntryData->content.append(content);
            LogMessage(QString("Content %1 added to the entry.").arg(content), LOG_NOTE);
            return true;
        case READ_QUOTE_DOUBLE:
            content.prepend('"');
            content.append('"');
            currEntryData->content.append(content);
            LogMessage(QString("Content %1 added to the entry.").arg(content), LOG_NOTE);
            return true;
        case READ_COMMENT_SINGLE:
            content.prepend("//");
            // content.append(newLine);
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
        //case READ_ENTRY_COMPLEX:
            LogMessage(QString("Complex content %4 of entry %1 of row %2 of table %3.").arg(currRow->entries.size()).arg(currTable->rows.size()).arg(currTable->name).arg(content), LOG_NOTE);
            if (currState.first == READ_ENTRY_COMPLEX) {
                /*if (!currRowEntry->postContent.isEmpty()) {
                    currRowEntry->content.append(currRowEntry->postContent);
                    currRowEntry->postContent.clear();
                }
                currRowEntry->content.append(content);*/
                currEntryData->content.append(content);
                currRowEntry->datas.push_back(currEntryData);
                currRowEntry->dataTexts.push_back(QString());
                currEntryData = nullptr;
                LogMessage(QString("Entry added to a complex entry."), LOG_NOTE);
                return true;
            }
            // fallthrough
        default:
            LogMessage(QString("Invalid type (%1) when reading entry content in state %2.").arg(type).arg(currState.first), LOG_ERROR);
            return false;
        }
        if (currEntryData->content.isEmpty()) {
            currEntryData->preContent.append(content);
        } else {
            currEntryData->postContent.append(content);
        }
        LogMessage(QString("Entry comment %1.").arg(content), LOG_NOTE);
        break;
    case READ_ENTRY_COMPLEX_POST:
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
        default:
            LogMessage(QString("Invalid type (%1) when reading entry content in state %2.").arg(type).arg(currState.first), LOG_ERROR);
            return false;
        }
        currRowEntry->dataTexts.back().append(content);
        LogMessage(QString("Entry post-comment %1.").arg(content), LOG_NOTE);
        break;
    default:
        LogMessage(QString("Unhandled entry content in %2 (type: %1).").arg(type).arg(currState.first), LOG_ERROR);
        return false;
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
            static const char *prefixes[] = { "const", "static", "constexpr" };
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
                // content.remove(0, newLine.length());
                if (!processContent(currState.first)) {
                    return false;
                }
                continue;
            }
            if (content.length() < 2 && newLine.length() >= 2) {
                return true;
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
                    states.push(std::pair<int, QString>(READ_TABLE, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
                    continue;
                }
            }
            if (content[0] == '#') {
                content.remove(0, 1);
                states.push(std::pair<int, QString>(READ_TABLE, "")); // currState.first
                currState.first = READ_DIRECTIVE;
                currState.second.clear(); // TODO: store the spaces?
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
                states.push(std::pair<int, QString>(READ_TABLE, "")); // currState.first
                currState.first = READ_ROW_COMPLEX;
                currState.second.clear(); // TODO: store the spaces?
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

                    states.push(std::pair<int, QString>(READ_ROW_SIMPLE, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
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
LogMessage(QString("Starting complex entry in a simple row %1.").arg(content), LOG_NOTE);
                content.remove(0, 1);
                initRowEntry();

                states.push(std::pair<int, QString>(READ_ROW_SIMPLE, "")); // currState.first
                currState.first = READ_ENTRY_COMPLEX;
                currState.second.clear(); // TODO: store the spaces?
                continue;
            }
            if (!content[0].isSpace()) {
                initRowEntry();
                currEntryData = new D1CppEntryData(); // initEntryData

                states.push(std::pair<int, QString>(READ_ROW_SIMPLE, "")); // currState.first
                currState.first = READ_ENTRY_SIMPLE;
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

                    states.push(std::pair<int, QString>(READ_ROW_COMPLEX, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
                    continue;
                }
            }
            if (content[0] == '}') {
LogMessage(QString("Finishing a complex entry %1.").arg(content), LOG_NOTE);
                content.remove(0, 1);
                /*if (!processContent(READ_ROW_COMPLEX)) {
                    return false;
                }*/
                // states.push(currState);
                currState.first = READ_ROW_COMPLEX_POST;
                currState.second.clear(); // TODO: store the spaces?
                continue;
            }
            if (content[0] == '{') {
LogMessage(QString("Starting complex entry in a complex row %1.").arg(content), LOG_NOTE);
                content.remove(0, 1);
                initRowEntry();

                states.push(std::pair<int, QString>(READ_ROW_COMPLEX, "")); // currState.first
                currState.first = READ_ENTRY_COMPLEX;
                currState.second.clear(); // TODO: store the spaces?
                continue;
            }
            if (!content[0].isSpace()) {
LogMessage(QString("Starting simple entry in a complex row %1.").arg(content), LOG_NOTE);
                initRowEntry();
                currEntryData = new D1CppEntryData(); // initEntryData

                states.push(std::pair<int, QString>(READ_ROW_COMPLEX, "")); // currState.first
                currState.first = READ_ENTRY_SIMPLE;
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
                    states.push(std::pair<int, QString>(READ_ROW_COMPLEX_POST, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
                    continue;
                }
            }
            if (content[0] == ',') {
LogMessage(QString("Starting post comment of a complex entry %1.").arg(content), LOG_NOTE);
                content.remove(0, 1);
                /*if (!processContent(READ_ROW_COMPLEX)) { // READ_ROW_COMPLEX_POST?
                    return false;
                }*/
                currState.first = READ_ROW_COMPLEX_POST_COMMENT;
                currState.second.clear(); // TODO: store the spaces?
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
        case READ_ROW_COMPLEX_POST_COMMENT:
            // TODO: better solution?
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(std::pair<int, QString>(READ_ROW_COMPLEX_POST_COMMENT, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
                    continue;
                }
            }
            if (content.startsWith(newLine)) {
                // content.remove(0, newLine.length());
                if (!processContent(READ_ROW_COMPLEX)) { // READ_ROW_COMPLEX_POST_COMMENT?
                    return false;
                }
                continue;
            }
            if (content.length() < 2 && newLine.length() >= 2) {
                return true;
            }
            if (content[0] == '}') {
                if (!processContent(READ_ROW_COMPLEX)) { // READ_ROW_COMPLEX_POST_COMMENT?
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
            if (content[0] == '"' || content[0] == '\'') {
                bool single = content[0] == '\'';
                content.remove(0, 1);
                states.push(currState);
                currState.first = single ? READ_QUOTE_SINGLE : READ_QUOTE_DOUBLE;
                currState.second = "";
                LogMessage(QString("Starting entry in quote %1.").arg(content), LOG_NOTE);
                continue;
            }
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(std::pair<int, QString>(READ_ENTRY_SIMPLE, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
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
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(std::pair<int, QString>(READ_ENTRY_COMPLEX, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
                    continue;
                }
            }
            /*if (content[0] == '{') {
                content.remove(0, 1);
                initRowEntry();

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
                content.remove(0, 1);
                /*if (!processContent(READ_ROW_COMPLEX)) {
                    return false;
                }*/
                // states.push(currState);
                currState.first = READ_ENTRY_COMPLEX_POST;
                continue;
            }
            if (!content[0].isSpace()) {
LogMessage(QString("Starting simple entry in a complex entry %1.").arg(content), LOG_NOTE);
                currEntryData = new D1CppEntryData(); // initEntryData

                states.push(std::pair<int, QString>(READ_ENTRY_COMPLEX, "")); // currState.first
                currState.first = READ_ENTRY_SIMPLE;
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
        case READ_ENTRY_COMPLEX_POST:
            // LogMessage(QString("Processing complex entry %1.").arg(content), LOG_NOTE);
            if (content[0] == '/') {
                if (content.length() < 2) {
                    return true;
                }
                if (content[1] == '/' || content[1] == '*') {
                    bool single = content[1] == '/';
                    content.remove(0, 2);
                    states.push(std::pair<int, QString>(READ_ENTRY_COMPLEX_POST, "")); // currState.first
                    currState.first = single ? READ_COMMENT_SINGLE : READ_COMMENT_MULTI;
                    currState.second.clear(); // TODO: store the spaces?
                    continue;
                }
            }
            if (content[0] == ',') {
                content.remove(0, 1);
                if (!processContent(READ_ENTRY_COMPLEX)) { // READ_ENTRY_COMPLEX_POST?
                    return false;
                }
                continue;
            }
            if (content[0] == '}') {
                if (!processContent(READ_ENTRY_COMPLEX)) { // READ_ENTRY_COMPLEX_POST?
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
        }
    }
    return true;
}

QString D1CppEntryData::getContent() const
{
    return this->content;
}

bool D1CppEntryData::setContent(const QString &text)
{
    if (this->content == text) {
        return false;
    }
    this->content = text;
    return true;
}

D1CppRowEntry::D1CppRowEntry(const QString &text)
{
    D1CppEntryData *data = new D1CppEntryData();
    data->setContent(text);
    this->datas.push_back(data);
    this->dataTexts.push_back(QString());
    this->dataTexts.push_back(QString());
}

D1CppRowEntry::~D1CppRowEntry()
{
    qDeleteAll(this->datas);
    this->datas.clear();
}

QString D1CppRowEntry::getContent() const
{
    return this->datas[0]->getContent();
}

bool D1CppRowEntry::setContent(const QString &text)
{
    return this->datas[0]->setContent(text);
}

bool D1CppRowEntry::isComplexFirst() const
{
    return this->complexFirst;
}

void D1CppRowEntry::setComplexFirst(bool complex)
{
    this->complexFirst = complex;
}

bool D1CppRowEntry::isComplexLast() const
{
    return this->complexLast;
}

void D1CppRowEntry::setComplexLast(bool complex)
{
    this->complexLast = complex;
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

void D1CppRow::insertEntry(int index)
{
    this->entries.insert(this->entries.begin() + index, new D1CppRowEntry(""));
    this->entryTexts.insert(this->entryTexts.begin() + index, QString());
}

void D1CppRow::duplicateEntry(int index)
{
    this->entries.insert(this->entries.begin() + index, new D1CppRowEntry(this->entries[index]->getContent()));
    this->entryTexts.insert(this->entryTexts.begin() + index, QString()/* this->entryTexts[index]*/);
}

void D1CppRow::trimEntry(int index)
{
    QString content = this->entries[index]->getContent();
    this->entries[index]->setContent(content.trimmed());
}

void D1CppRow::delEntry(int index)
{
    QString text = this->entryTexts.takeAt(index);
    D1CppRowEntry *entry = this->entries.takeAt(index);

    if (entry->isComplexFirst()) {
        if (entry->isComplexLast()) {
            ; // do nothing
        } else if (this->entries.count() > index) {
            D1CppRowEntry *nextEntry = this->entries[index];
            nextEntry->setComplexFirst(true);
        }
    } else if (entry->isComplexLast()) {
        if (index != 0) {
            D1CppRowEntry *prevEntry = this->entries[index - 1];
            prevEntry->setComplexLast(true);
        }
    }

    delete entry;
    /*if (!this->entryTexts[index].isEmpty()) {
        this->entryTexts[index].prepend(lineEnd);
    }*/
    this->entryTexts[index].prepend(text);
}

void D1CppRow::swapColumns(int index1, int index2, bool complete)
{
    this->entries.swapItemsAt(index1, index2);
    if (complete) {
        this->entryTexts.swapItemsAt(index1, index2);
    }
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
    return this->rows.isEmpty() ? this->header.count() : this->rows[0]->entries.count();
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

bool D1CppTable::setHeader(int index, const QString &text)
{
    if (this->header[index] == text) {
        return false;
    }
    this->header[index] = text;
    return true;
}

D1CPP_ENTRY_TYPE D1CppTable::getColumnType(int index) const
{
    return this->columnType[index];
}

bool D1CppTable::setColumnType(int index, D1CPP_ENTRY_TYPE type)
{
    if (this->columnType[index] == type) {
        return false;
    }
    this->columnType[index] = type;
    // TODO: set/check type of the D1CppEntryData?
    return true;
}

QString D1CppTable::getLeader(int index) const
{
    return this->leader[index];
}

bool D1CppTable::setLeader(int index, const QString &text)
{
    if (this->leader[index] == text) {
        return false;
    }
    this->leader[index] = text;
    return true;
}

void D1CppTable::insertRow(int index)
{
    D1CppRow *row = new D1CppRow();
    int entries = this->getColumnCount();
    for (int i = 0; i < entries; i++) {
        row->insertEntry(0);
    }

    this->rows.insert(this->rows.begin() + index, row);
    this->rowTexts.insert(this->rowTexts.begin() + index, QString());
    this->leader.insert(this->leader.begin() + index, QString());
}

void D1CppTable::duplicateRow(int index)
{
    D1CppRow *rowbase = this->rows[index];
    D1CppRow *row = new D1CppRow();
    int entries = this->getColumnCount();
    for (int i = 0; i < entries; i++) {
        row->insertEntry(0);
    }
    for (int i = 0; i < entries; i++) {
        row->getEntry(i)->setContent(rowbase->getEntry(i)->getContent());
        // row->getEntry(i)->setEntryText(rowbase->getEntryText());
    }

    this->rows.insert(this->rows.begin() + index, row);
    this->rowTexts.insert(this->rowTexts.begin() + index, this->rowTexts[index]);
    this->leader.insert(this->leader.begin() + index, this->leader[index]);
}

void D1CppTable::swapRows(int index1, int index2, bool complete)
{
    this->rows.swapItemsAt(index1, index2);
    this->leader.swapItemsAt(index1, index2);
    if (complete) {
        this->rowTexts.swapItemsAt(index1, index2);
    }
}

void D1CppTable::swapColumns(int index1, int index2, bool complete)
{
    for (D1CppRow *row : this->rows) {
        row->swapColumns(index1, index2, complete);
    }
    this->header.swapItemsAt(index1, index2);
    this->columnType.swapItemsAt(index1, index2);
}

void D1CppTable::insertColumn(int index)
{
    for (D1CppRow *row : this->rows) {
        row->insertEntry(index);
    }
    this->header.insert(this->header.begin() + index, QString());
    this->columnType.insert(this->columnType.begin() + index, D1CPP_ENTRY_TYPE::String);
}

void D1CppTable::duplicateColumn(int index)
{
    for (D1CppRow *row : this->rows) {
        row->duplicateEntry(index);
    }
    this->header.insert(this->header.begin() + index, this->header[index]);
    this->columnType.insert(this->columnType.begin() + index, this->columnType[index]);
}

void D1CppTable::trimColumn(int index)
{
    for (D1CppRow *row : this->rows) {
        row->trimEntry(index);
    }
}

void D1CppTable::delRow(int index)
{
    QString text = this->rowTexts.takeAt(index);
    D1CppRow *row = this->rows.takeAt(index);
    this->leader.takeAt(index);

    delete row;
    /*if (!this->rowTexts[index].isEmpty()) {
        this->rowTexts[index].prepend(lineEnd);
    }*/
    this->rowTexts[index].prepend(text);
}

void D1CppTable::delColumn(int index)
{
    for (D1CppRow *row : this->rows) {
        row->delEntry(index);
    }
    this->header.takeAt(index);
    this->columnType.takeAt(index);
}

D1Cpp::~D1Cpp()
{
    qDeleteAll(this->tables);
    this->tables.clear();
}

static int arrayLen(QString &header)
{
    if (!header.endsWith(']'))
        return -1;
    int pos = header.lastIndexOf('[');
    if (pos == -1)
        return -1;
    bool ok;
    int num = header.mid(pos + 1, header.length() - (pos + 1 + 1)).toInt(&ok);
    if (!ok || num == 0)
        return -1;
    return num;
}

bool D1Cpp::postProcess()
{
    bool change = false;
    for (D1CppTable *table : this->tables) {
        LogMessage(QString("Found table %1: %2 x %3.").arg(table->getName()).arg(table->getRowCount()).arg(table->getColumnCount()), LOG_NOTE);
        // flatten the table
        for (int i = 0; i < table->getRowCount(); i++) {
            D1CppRow *row = table->getRow(i);
            for (int e = 0; e < row->entries.count(); e++) {
                D1CppRowEntry *entry = row->entries[e];
                int d = entry->datas.count();
                if (d == 1) {
                    continue;
                }
                if (d == 0) {
                    // empty row-data
                    entry->datas.push_back(new D1CppEntryData());
                    entry->dataTexts.push_back(QString());
                    continue;
                }
        LogMessage(QString("Splitting colum %1 of row %2 with %3 data-entries.").arg(e).arg(i).arg(d), LOG_NOTE);
                entry->complexFirst = true;
                e++;
                bool complexLast = true;
                for (d--; d > 0; d--) {
                    D1CppEntryData *data = entry->datas.last();
                    entry->datas.pop_back();
                    int sn = entry->dataTexts.count() - 2;
                    QString dataStr = entry->dataTexts[sn];
                    entry->dataTexts.erase(entry->dataTexts.begin() + sn);

                    D1CppRowEntry *flatEntry = new D1CppRowEntry();
                    flatEntry->complexLast = complexLast;
                    flatEntry->datas.push_back(data);
                    flatEntry->dataTexts.push_back(dataStr);
                    flatEntry->dataTexts.push_back(QString());
                    row->entries.insert(row->entries.begin() + e, flatEntry);
                    row->entryTexts.insert(row->entryTexts.begin() + e, QString());

                    complexLast = false;
                }
            }
        }
        LogMessage(QString("Flat tablesize %1 x %2.").arg(table->getRowCount()).arg(table->getColumnCount()), LOG_NOTE);
        // balance the table
        int maxColumnNum = 0;
        bool ch = false;
        for (int i = 0; i < table->getRowCount(); i++) {
            D1CppRow *row = table->getRow(i);
            int cc = row->entries.count();
            if (i == 0 || maxColumnNum != cc) {
        if (i != 0)
        LogMessage(QString("Unbalanced table row %1: %2 vs. %3.").arg(i).arg(cc).arg(maxColumnNum), LOG_NOTE);
                if (maxColumnNum < cc) {
                    maxColumnNum = cc;
                }
                ch = i != 0;
            }
        }
        if (ch) {
            change = true;
            dProgressWarn() << tr("Entries added to unbalanced table %1").arg(table->getName());
            for (int i = 0; i < table->getRowCount(); i++) {
                D1CppRow *row = table->getRow(i);
                while (row->entries.count() < maxColumnNum) {
                    row->entries.push_back(new D1CppRowEntry(""));
                    row->entryTexts.push_back(QString());
                }
            }
        }

        // split leader fields
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
                    firstText = firstText.left(x - 1);
                }

            }
            table->leader.push_back(rowLeader);
        }
        // add header fields
        if (!table->rowTexts.isEmpty()) {
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
                            QString header = headerName.trimmed();
                            table->header.push_back(header);

                            // add header to arrays
                            int num = arrayLen(header);
                            for (int i = 0; i < num - 1; i++) {
                                table->header.push_back(QString());
                            }
                        }
                        if (table->header[0].isEmpty()) {
                            table->header[0] = " ";
                        }
                    }
                }
        LogMessage(QString("Resulting firstText %1.").arg(table->rowTexts[0]), LOG_NOTE);
            }
        }
        // balance headers with the content content
        LogMessage(QString("Header count %1 columns %2.").arg(table->header.count()).arg(table->getColumnCount()), LOG_NOTE);
        int num = table->getColumnCount() - table->header.count();
        for (int i = 0; i < num; i++) {
            table->header.push_back(QString());
        }
        for (int i = 0; i < -num; i++) {
            QString lastHeader = table->header.back();
            table->header.pop_back();
            lastHeader.prepend(", ");
            if (!table->header.isEmpty()) {
                table->header.back().append(lastHeader);
            } else {
                LogMessage(QString("Restored row-text '%1' due to missing columns.").arg(lastHeader), LOG_NOTE);
                table->rowTexts[0].prepend(lastHeader);
            }
        }
        for (int i = 0; i < table->getColumnCount(); i++) {
            table->columnType.push_back(D1CPP_ENTRY_TYPE::String);
        }
        // trim content + set types
        for (int i = table->getColumnCount() - 1; i >= 0; i--) {
            // trim content
            bool isNumber = true;
            bool isReal = true;
            bool isQoutedString = true;
            bool isOneWord = true;
            for (D1CppRow *row : table->rows) {
                D1CppRowEntry *entry = row->entries[i];
                QString content = entry->datas[0]->content.trimmed();
                isQoutedString &= content.startsWith('"') && content.endsWith('"');
                isOneWord &= content.indexOf(' ') == -1;
                if (isNumber) {
                    content.toInt(&isNumber);
                }
                if (isReal) {
                    content.toDouble(&isReal);
                }
            }
LogMessage(QString("Column %1 (%2 of %3): num: %4 real: %5 isQoutedString: %6 isOneWord: %7.").arg(i).arg(table->header[i]).arg(table->getName()).arg(isNumber).arg(isReal).arg(isQoutedString).arg(isOneWord), LOG_NOTE);
            D1CPP_ENTRY_TYPE type = D1CPP_ENTRY_TYPE::String;
            if (isNumber) {
                type = D1CPP_ENTRY_TYPE::Integer;
            } else if (isReal) {
                type = D1CPP_ENTRY_TYPE::Real;
            } else if (isQoutedString) {
                type = D1CPP_ENTRY_TYPE::QuotedString;
            }
            table->columnType[i] = type;
            int leadingSpaces = INT_MAX, trailingSpaces = INT_MAX;
            for (D1CppRow *row : table->rows) {
                D1CppEntryData *data = row->entries[i]->datas[0];
                data->type = type;
                if (type != D1CPP_ENTRY_TYPE::String || isOneWord) {
                    data->content = data->content.trimmed();
                } else {
                    int ls = 0;
                    for ( ; ls < data->content.length(); ls++) {
                        if (data->content[ls] != ' ') {
                            break;
                        }
                    }
                    if (ls < leadingSpaces) {
                        leadingSpaces = ls;
                    }
                    int ts = data->content.length();
                    for ( ; ts > 0; ts--) {
                        if (data->content[ts - 1] != ' ') {
                            break;
                        }
                    }
                    ts = data->content.length() - ts;
                    if (ts < trailingSpaces) {
                        trailingSpaces = ts;
                    }
                }
            }
            if (type != D1CPP_ENTRY_TYPE::String || isOneWord) {
                continue;
            }
LogMessage(QString("Column %1 (%2 of %3): type: %4 leadingspaces: %5 trailingSpaces: %6 isQoutedString: %7.").arg(i).arg(table->header[i]).arg(table->getName()).arg((int)type).arg(leadingSpaces).arg(trailingSpaces), LOG_NOTE);
            // trim common spaces based on the rows
            for (D1CppRow *row : table->rows) {
                D1CppEntryData *data = row->entries[i]->datas[0];
                if (leadingSpaces != 0) {
                    data->content = data->content.mid(leadingSpaces);
                }
                if (trailingSpaces != 0) {
                    data->content.chop(trailingSpaces);
                }
            }
            // TODO: trim spaces based on the previous column?
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

    // validate the content
    for (D1CppTable *table : this->tables) {
        for (int n = 0; n < table->header.count(); n++) {
            QString &header = table->header[n];
            int num = arrayLen(header);
            /*if (num < 0) {
                continue;
            }
            if (table->header.count() < n + num) {
                dProgressErr() << tr("Invalid header '%1' in table %2. (Not enough headers)").arg(header).arg(table->getName());
                return false;
            }*/
            for (int i = 1; i < num; i++) {
                if (table->header.count() > n + i && !table->header[n + i].isEmpty()) {
                    dProgressErr() << tr("Invalid header '%1' in table %2. (Followup headers are not empty)").arg(header).arg(table->getName());
                    return false;
                }
            }
        }
    }

    // open the file for write
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    QTextStream out(&outFile);
    // write the file
    int i = 0;
    for ( ; i < this->tables.count(); i++) {
        out << this->texts[i];

        out << "{";
        out << this->lineEnd;

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
        QList<QList<QPair<QString, int>>> entryContents;
        for (const QString &header : table->header) {
            maxWidths.push_back(0);
        }
        for (int n = 0; n < table->rows.count(); n++) {
            QList<QPair<QString, int>> rowEntryContents;
            const D1CppRow *row = table->rows[n];
            int w = 0;
            for (D1CppRowEntry *entry : row->entries) {
                // TODO: handle row->entryTexts[e]
                QString entryContent;
                for (int a = 0; a < entry->datas.count(); a++) {
                    D1CppEntryData *data = entry->datas[a];
                    w++;

                    entryContent += data->preContent + data->content + data->postContent;
                }
                int len = entryContent.length();
                int complex = 0;
                if (entry->isComplexFirst()) {
                    complex |= 1;
                    len += 2;
                }
                if (entry->isComplexLast()) {
                    len += 2;
                    complex |= 2;
                }
                rowEntryContents.push_back(QPair<QString, int>(entryContent, complex));
                if (maxWidths[w] < len) {
                    maxWidths[w] = len;
                }
            }
            entryContents.push_back(rowEntryContents);
        }
        QList<int> headerWidths = maxWidths;
        int w = 0;
        for (int h = 0; h < table->header.count(); h++) {
            QString &header = table->header[h];
            w++;

            int len = header.length();
            if (headerWidths.count() <= w) {
    LogMessage(QString("Missing content for header %1.").arg(w), LOG_NOTE);
                headerWidths.push_back(len);
                continue;
            }
            int num = arrayLen(header);
            if (num < 0) {
    LogMessage(QString("Standard header %1 with %2 vs %3.").arg(header).arg(len).arg(maxWidths[w]), LOG_NOTE);
                if (maxWidths[w] < len) {
                    maxWidths[w] = len;
                    headerWidths[w] = len;
                }
            } else {
                // calculate the width of the content
                int mw = 2 * (num - 1);
                for (int n = 0; n < num; n++) {
                    if (maxWidths.count() > w + n) {
                        mw += maxWidths[w + n];
                    } else {
                        mw -= 2;
                    }
                }
    LogMessage(QString("Array header %1 with %2 vs %3 (%4).").arg(header).arg(len).arg(mw).arg(num), LOG_NOTE);
                if (mw < len) {
                    // raise content width (if there is)
                    if (maxWidths.count() > w + num - 1) {
    LogMessage(QString("Raised content %1 with %2.").arg(w + num - 1).arg(len - mw), LOG_NOTE);
                        maxWidths[w + num - 1] += len - mw;
                    }

                    mw = len;
                }
                // set the width of the first header
                headerWidths[w] = mw;
                // reset the followup headers
                for (int n = 1; n < num; n++) {
    LogMessage(QString("Array header marked for skipping %1.").arg(w + n), LOG_NOTE);
                    if (headerWidths.count() > w + n) {
                        headerWidths[w + n] = -1;
                    } else {
                        headerWidths.push_back(-1);
                    }
                }
                // skip the followup headers
                h += num - 1;
                w += num - 1;
            }            
        }
        // TODO: if (table->header.count() != 0) {
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
                    if (!last && headerWidths[h + 1] != -1) {
                        content += ", ";
                        int width = headerWidths[h + 1] + 2;
                        if ((table->columnType[h] == D1CPP_ENTRY_TYPE::Integer || table->columnType[h] == D1CPP_ENTRY_TYPE::Real) && headerWidths[h + 2] != -1) {
                            content = content.rightJustified(width, ' ');
                        } else {
                            content = content.leftJustified(width, ' ');
                        }
} else if (headerWidths[h + 1] == -1) {
        LogMessage(QString("Array header skipping %1.").arg(h + 1), LOG_NOTE);
                    }
                    out << content;
                    if (last) {
                        break;
                    }
                }
                out << this->lineEnd;
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
            while (true) {
                /*out << row->entryTexts[e];
                out << row->entries[e]->preContent;
                out << row->entries[e]->content;
                out << row->entries[e]->postContent;*/
                QPair<QString, int> &entry = entryContents[n][e];
                QString &content = entry.first;
                bool isComplexFirst = (entry.second & 1) != 0;
                bool isComplexLast = (entry.second & 2) != 0;
                bool last = e == entryContents[n].count() - 1;
                if (!last && !isComplexLast) {
                    content += ", ";
                }
                int width = maxWidths[e + 1] + (last ? 0 : 2) + (isComplexLast ? -4 : 0) + (isComplexFirst ? -2 : 0);
                if (table->columnType[e] == D1CPP_ENTRY_TYPE::Integer || table->columnType[e] == D1CPP_ENTRY_TYPE::Real) {
                    content = content.rightJustified(width, ' ');
                } else {
                    content = content.leftJustified(width, ' ');
                }
                if (isComplexFirst) {
                    content.prepend("{ ");
                }
                if (isComplexLast) {
                    content += " }";
                    if (!last) {
                        content += ", ";
                    }
                }
                out << content;
                e++;
                if (last) {
                    break;
                }
            }
            out << " }"; // if row->isComplex
            out << ",";
            if (!table->rows[n]->entryTexts[e].isEmpty()) {
                out << " ";
                out << table->rows[n]->entryTexts[e];
            }
            out << this->lineEnd;
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

