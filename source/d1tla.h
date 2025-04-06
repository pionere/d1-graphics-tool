#pragma once

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Tla : public QObject {
    Q_OBJECT

public:
    D1Tla() = default;
    ~D1Tla() = default;

    bool load(const QString &tlaFilePath, int tileCount, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void clear();

    void compareTo(const D1Tla *tla) const;

    QString getFilePath() const;
    bool isModified() const;
    quint8 getTileProperties(int tileIndex) const;
    bool setTileProperties(int tileIndex, quint8 value);
    void insertTile(int tileIndex);
    void removeTile(int tileIndex);

private:
    QString tlaFilePath;
    bool modified;
    QList<quint8> properties;
};
