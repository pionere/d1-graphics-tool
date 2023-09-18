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

class D1CppRowEntry : public QObject {
    Q_OBJECT

    friend class D1Cpp;

public:
    D1CppRowEntry() = default;
    ~D1CppRowEntry() = default;

    QString getContent() const;

private:
    QString preContent;

    QString content;
    D1CPP_ENTRY_TYPE type;

    QString postContent;
};

class D1CppRow : public QObject {
    Q_OBJECT

    friend class D1Cpp;
    friend class D1CppTable;

public:
    D1CppRow() = default;
    ~D1CppRow();

    D1CppRowEntry *getEntry(int index) const;

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

private:
    QString name;

    QList<QString> rowTexts;
    QList<D1CppRow *> rows;
};

class D1Cpp : public QObject {
    Q_OBJECT

public:
    D1Cpp() = default;
    ~D1Cpp();

    bool load(const QString &filePath);
    bool save(const SaveAsParam &params);

    bool isModified() const;

    QString getFilePath() const;
    void setFilePath(const QString &path);
    int getTableCount() const;
    D1CppTable *getTable(int index) const;

private:
    bool processContent(QString &content, int type);
    bool readContent(QString &content);
    bool postProcess();
    static void initRow();
    static bool initTable();

private:
    QString cppFilePath;
    bool modified;

    QString lineEnd;
    QList<QString> texts;
    QList<D1CppTable *> tables;
};
