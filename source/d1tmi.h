#pragma once

#include <map>

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Sol;

class D1Tmi : public QObject {
    Q_OBJECT

public:
    D1Tmi() = default;
    ~D1Tmi() = default;

    bool load(const QString &tmiFilePath, const D1Sol *sol, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void clear();

    void insertSubtile(int subtileIndex, quint8 value);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);

    QString getFilePath() const;
    bool isModified() const;
    quint8 getSubtileProperties(int subtileIndex) const;
    bool setSubtileProperties(int subtileIndex, quint8 value);

private:
    QString tmiFilePath;
    bool modified;
    QList<quint8> subProperties;
};
