#pragma once

#include <QList>
#include <QMap>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Sol;

class D1Tmi : public QObject {
    Q_OBJECT

public:
    D1Tmi() = default;
    ~D1Tmi() = default;

    bool load(QString filePath, D1Sol *sol, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    void insertSubtile(int subtileIndex, quint8 value);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(QMap<unsigned, unsigned> &remap);

    QString getFilePath();
    quint8 getSubtileProperties(int subtileIndex);
    void setSubtileProperties(int subtileIndex, quint8 value);

private:
    QString tmiFilePath;
    QList<quint8> subProperties;
};
