#pragma once

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Tit : public QObject {
    Q_OBJECT

public:
    D1Tit() = default;
    ~D1Tit() = default;

    bool load(const QString &titFilePath, int tileCount, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void clear();

    QString getFilePath() const;
    bool isModified() const;
    quint8 getTileProperties(int tileIndex) const;
    bool setTileProperties(int tileIndex, quint8 value);
    void insertTile(int tileIndex);
    void createTile();
    void removeTile(int tileIndex);

private:
    QString titFilePath;
    bool modified;
    QList<quint8> properties;
};
