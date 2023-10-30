#pragma once

#include <vector>

#include <QFile>
#include <QObject>
#include <QString>

#include "saveasdialog.h"

enum class D1CPP_ENTRY_TYPE {
    Header,
    Enum,
    Integer,
    Real,
    String,
    QuotedString,
    Char,
    QuotedChar,
};

class D1CppEntryData : public QObject {
    Q_OBJECT

    friend class D1Cpp;
    friend class D1CppRowEntry;

public:
    D1CppEntryData() = default;
    ~D1CppEntryData() = default;

    QString getContent() const;
    void setContent(const QString &text);

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
    ~D1CppRowEntry();

    QString getContent() const;
    void setContent(const QString &text);

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
	void delEntry(int column);

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
    QString getLeader(int index) const;
	void insertRow(int row);
	void insertColumn(int column);
	void delRow(int row);
	void delColumn(int column);

private:
    QString name;

    QList<QString> rowTexts;
    QList<D1CppRow *> rows;
    QList<QString> header;
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