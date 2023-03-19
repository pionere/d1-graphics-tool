#pragma once

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Amp : public QObject {
    Q_OBJECT

public:
    D1Amp() = default;
    ~D1Amp() = default;

    bool load(const QString &ampFilePath, int tileCount, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void clear();

    QString getFilePath() const;
    bool isModified() const;
    quint8 getTileType(int tileIndex) const;
    quint8 getTileProperties(int tileIndex) const;
    bool setTileType(int tileIndex, quint8 value);
    bool setTileProperties(int tileIndex, quint8 value);
    void insertTile(int tileIndex);
    void createTile();
    void removeTile(int tileIndex);

private:
    QString ampFilePath;
    bool modified;
    QList<quint8> types;
    QList<quint8> properties;
};
