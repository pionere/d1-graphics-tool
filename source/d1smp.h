#pragma once

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Smp : public QObject {
    Q_OBJECT

public:
    D1Smp() = default;
    ~D1Smp() = default;

    bool load(const QString &smpFilePath, int subtileCount, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void clear();

    QString getFilePath() const;
    bool isModified() const;
    quint8 getSubtileType(int subtileIndex) const;
    quint8 getSubtileProperties(int subtileIndex) const;
    bool setSubtileType(int subtileIndex, quint8 value);
    bool setSubtileProperties(int subtileIndex, quint8 value);

    void insertSubtile(int subtileIndex);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);

private:
    QString smpFilePath;
    bool modified;
    QList<quint8> types;
    QList<quint8> properties;
};
