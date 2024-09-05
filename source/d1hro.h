#pragma once

#include <QFile>
#include <QString>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Hero {
    Q_OBJECT

public:
    ~D1Hero();

    static D1Hero* instance();

    bool load(const QString &filePath, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    void compareTo(const D1Hero *hero, QString header) const;

    QString getFilePath() const;
    void setFilePath(const QString &filePath);
    bool isModified() const;
    void setModified(bool modified = true);

private:
    D1Hero() = default;

    QString filePath;
    int pnum;
    bool modified = false;
};
