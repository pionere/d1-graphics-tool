#pragma once

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Tileset;

class D1Sla : public QObject {
    Q_OBJECT

public:
    D1Sla() = default;
    ~D1Sla() = default;

    bool load(const QString &slaFilePath);
    bool save(const SaveAsParam &params);
    void clear();

    void compareTo(const D1Sla *sla) const;

    QString getFilePath() const;
    bool isModified() const;
    int getSubtileCount() const;
    quint8 getSubProperties(int subtileIndex) const;
    bool setSubProperties(int subtileIndex, quint8 value);
    quint8 getLightRadius(int subtileIndex) const;
    bool setLightRadius(int subtileIndex, quint8 value);
    int getTrapProperty(int subtileIndex) const;
    bool setTrapProperty(int subtileIndex, int value);
    int getSpecProperty(int subtileIndex) const;
    bool setSpecProperty(int subtileIndex, int value);
    quint8 getRenderProperties(int subtileIndex) const;
    bool setRenderProperties(int subtileIndex, quint8 value);
    quint8 getMapType(int subtileIndex) const;
    bool setMapType(int subtileIndex, quint8 value);
    quint8 getMapProperties(int subtileIndex) const;
    bool setMapProperties(int subtileIndex, quint8 value);

    void insertSubtile(int subtileIndex);
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);
    void resetSubtileFlags(int subtileIndex);

private:
    QString slaFilePath;
    bool modified;
    QList<quint8> subProperties;
    QList<quint8> lightRadius;
    QList<int> trapProperties;
    QList<int> specProperties;
    QList<quint8> renderProperties;
    QList<quint8> mapTypes;
    QList<quint8> mapProperties;
};
