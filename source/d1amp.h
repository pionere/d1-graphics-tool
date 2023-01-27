#pragma once

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Amp : public QObject {
    Q_OBJECT

public:
    D1Amp() = default;
    ~D1Amp() = default;

    bool load(QString filePath, int tileCount, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    QString getFilePath();
    bool isModified() const;
    quint8 getTileType(int tileIndex);
    quint8 getTileProperties(int tileIndex);
    bool setTileType(int tileIndex, quint8 value);
    bool setTileProperties(int tileIndex, quint8 value);
    void createTile();
    void removeTile(int tileIndex);

private:
    QString ampFilePath;
    bool modified;
    QList<quint8> types;
    QList<quint8> properties;
};
