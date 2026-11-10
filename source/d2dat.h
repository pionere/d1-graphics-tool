#pragma once

#include <vector>

#include "d1pal.h"
#include "d1trn.h"

#define D2PAL_COLORS 256
#define D2PAL_SIZE_BYTES (D2PAL_COLORS * 3)

class D2Dat : public QObject {
    Q_OBJECT

public:
    D2Dat() = default;
    ~D2Dat() = default;

    static bool load(const QString &datFilePath, std::vector<D1Pal *> &pals, std::vector<D1Trn *> &trns);
    static bool save(const QString &datFilePath, const std::vector<D1Pal *> &pals);

private:
    void loadRegularPalettes(QFile &file, const QString &baseFilePath, std::vector<D1Pal *> &pals);
    bool loadTransformationPalette(QFile &file, const QString &baseFilePath, std::vector<D1Pal *> &pals, std::vector<D1Trn *> &trns);
};
