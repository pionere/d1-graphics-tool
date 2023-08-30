#pragma once

#include <vector>

#include "d1trn.h"

class GenerateTrnParam;

class D1Trs : public QObject {
    Q_OBJECT

public:
    D1Trs() = default;
    ~D1Trs() = default;

    static bool load(const QString &trsFilePath, D1Pal *pal, std::vector<D1Trn *> &trns);
    static bool save(const QString &trsFilePath, const std::vector<D1Trn *> &trns);

    static void generateLightTranslations(const GenerateTrnParam &params, std::vector<D1Trn *> &trns);
};
