#pragma once

#include <vector>

#include <QString>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Til;
class D1Tmi;

class D1Dun : public QObject {
    Q_OBJECT

public:
    D1Dun() = default;
    ~D1Dun() = default;

    bool load(const QString &dunFilePath, D1Til *til, D1Tmi *tmi, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    QString getFilePath() const;
    bool isModified() const;

private:
    QString dunFilePath;
    D1Til *til;
    D1Tmi *tmi;
    bool modified;
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<int>> items;
    std::vector<std::vector<int>> monsters;
    std::vector<std::vector<int>> objects;
    std::vector<std::vector<int>> transvals;
};
