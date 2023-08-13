#pragma once

#include <map>

#include <QList>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Spt : public QObject {
    Q_OBJECT

public:
    D1Spt() = default;
    ~D1Spt() = default;

    bool load(const QString &sptFilePath, int subtileCount, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void clear();

    void insertSubtile(int subtileIndex);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);

    QString getFilePath() const;
    bool isModified() const;
    int getSubtileTrapProperty(int subtileIndex) const;
    bool setSubtileTrapProperty(int subtileIndex, int value);
    int getSubtileSpecProperty(int subtileIndex) const;
    bool setSubtileSpecProperty(int subtileIndex, int value);

private:
    QString sptFilePath;
    bool modified;
    QList<int> trapProperties;
    QList<int> specProperties;
};
