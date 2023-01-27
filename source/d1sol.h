#pragma once

#include <QList>
#include <QMap>

#include "saveasdialog.h"

class D1Sol : public QObject {
    Q_OBJECT

public:
    D1Sol() = default;
    ~D1Sol() = default;

    bool load(QString filePath);
    bool save(const SaveAsParam &params);

    void insertSubtile(int subtileIndex, quint8 value);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const QMap<unsigned, unsigned> &remap);

    QString getFilePath();
    bool isModified() const;
    int getSubtileCount();
    quint8 getSubtileProperties(int subtileIndex);
    bool setSubtileProperties(int subtileIndex, quint8 value);

private:
    QString solFilePath;
    bool modified;
    QList<quint8> subProperties;
};
