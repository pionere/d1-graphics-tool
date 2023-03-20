#pragma once

#include <map>

#include <QList>

#include "saveasdialog.h"

class D1Sol : public QObject {
    Q_OBJECT

public:
    D1Sol() = default;
    ~D1Sol() = default;

    bool load(const QString &solFilePath);
    bool save(const SaveAsParam &params);
    void clear();

    void insertSubtile(int subtileIndex, quint8 value);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);

    QString getFilePath() const;
    bool isModified() const;
    int getSubtileCount() const;
    quint8 getSubtileProperties(int subtileIndex) const;
    bool setSubtileProperties(int subtileIndex, quint8 value);

private:
    QString solFilePath;
    bool modified;
    QList<quint8> subProperties;
};
