#pragma once

#include <vector>

#include <QFile>
#include <QObject>
#include <QString>

#include "saveasdialog.h"

enum class D1CPP_ENTRY_TYPE {
    String,
    QuotedString,
    // Char,
    // QuotedChar,
    // Enum,
    Integer,
    Real,
};

class D1CppEntryData : public QObject {
    Q_OBJECT

    friend class D1Cpp;
    friend class D1CppRowEntry;

public:
    D1CppEntryData() = default;
    ~D1CppEntryData() = default;

    QString getContent() const;
    bool setContent(const QString &text);

private:
    QString preContent;

    QString content;
    D1CPP_ENTRY_TYPE type;

    QString postContent;
};

class D1CppRowEntry : public QObject {
    Q_OBJECT

    friend class D1Cpp;

public:
    D1CppRowEntry() = default;
    D1CppRowEntry(const QString &text);
    ~D1CppRowEntry();

    QString getContent() const;
    bool setContent(const QString &text);
    bool isComplexFirst() const;
    void setComplexFirst(bool complex);
    bool isComplexLast() const;
    void setComplexLast(bool complex);

private:
    QList<QString> dataTexts;
    QList<D1CppEntryData *> datas;
    bool complexFirst = false;
    bool complexLast = false;
};

class D1CppRow : public QObject {
    Q_OBJECT

    friend class D1Cpp;
    friend class D1CppTable;

public:
    D1CppRow() = default;
    ~D1CppRow();

    D1CppRowEntry *getEntry(int index) const;
    QString getEntryText(int index) const;
    void insertEntry(int column);
    void trimEntry(int column);
    void delEntry(int column);
    void moveColumnLeft(int column, bool complete);
    void moveColumnRight(int column, bool complete);

private:
    QList<QString> entryTexts;
    QList<D1CppRowEntry *> entries;
};

class D1CppTable : public QObject {
    Q_OBJECT

    friend class D1Cpp;

public:
    D1CppTable(const QString &name);
    ~D1CppTable();

    QString getName() const;
    int getColumnCount() const;
    int getRowCount() const;
    D1CppRow *getRow(int index) const;
    QString getRowText(int index) const;
    QString getHeader(int index) const;
    bool setHeader(int index, const QString &text);
    D1CPP_ENTRY_TYPE getColumnType(int index) const;
    bool setColumnType(int index, D1CPP_ENTRY_TYPE type);
    QString getLeader(int index) const;
    void insertRow(int row);
    void moveRowUp(int row, bool complete);
    void moveRowDown(int row, bool complete);
    void insertColumn(int column);
    void trimColumn(int column);
    void moveColumnLeft(int column, bool complete);
    void moveColumnRight(int column, bool complete);
    void delRow(int row);
    void delColumn(int column);

private:
    QString name;

    QList<QString> header;
    QList<D1CPP_ENTRY_TYPE> columnType;

    QList<QString> rowTexts;
    QList<D1CppRow *> rows;
    QList<QString> leader;
};

class D1Cpp : public QObject {
    Q_OBJECT

public:
    D1Cpp() = default;
    ~D1Cpp();

    bool load(const QString &filePath);
    bool save(const SaveAsParam &params);

    bool isModified() const;
    void setModified();

    QString getFilePath() const;
    void setFilePath(const QString &path);
    int getTableCount() const;
    D1CppTable *getTable(int index) const;

private:
    bool processContent(int type);
    bool readContent(QString &content);
    bool postProcess();
    static void initRowEntry();
    static void initRow();
    static bool initTable();

private:
    QString cppFilePath;
    bool modified;

    QString lineEnd;
    QList<QString> texts;
    QList<D1CppTable *> tables;
};
