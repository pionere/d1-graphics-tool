#pragma once

#include <QImage>
#include <QList>

#include "d1min.h"
#include "saveasdialog.h"

#define TILE_WIDTH 2
#define TILE_HEIGHT 2

class D1Til : public QObject {
    Q_OBJECT

public:
    D1Til() = default;
    ~D1Til() = default;

    bool load(QString filePath, D1Min *min);
    bool save(const SaveAsParam &params);

    QImage getTileImage(int tileIndex) const;
    QImage getFlatTileImage(int tileIndex) const;
    void insertTile(int tileIndex, const QList<quint16> &subtileIndices);
    void createTile();
    void removeTile(int tileIndex);

    QString getFilePath() const;
    const D1Min *getMin() const;
    bool isModified() const;
    void setModified();
    int getTileCount() const;
    QList<quint16> &getSubtileIndices(int tileIndex) const;
    bool setSubtileIndex(int tileIndex, int index, int subtileIndex);

private:
    QString tilFilePath;
    bool modified;
    QList<QList<quint16>> subtileIndices;
    D1Min *min = nullptr;
};
