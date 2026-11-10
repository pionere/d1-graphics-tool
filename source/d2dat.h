#pragma once

#include <vector>

#include "d1pal.h"

#define D2PAL_COLORS 256

class D2Dat : public QObject {
    Q_OBJECT

public:
    D2Dat() = default;
    ~D2Dat() = default;

    static bool load(const QString &datFilePath, std::vector<D1Pal *> &pals);
    static bool save(const QString &datFilePath, const std::vector<D1Pal *> &pals);
};
