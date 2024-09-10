/**
 * @file lighting.cpp
 *
 * Implementation of light and vision.
 */
#include "all.h"

#include <QString>

#include "../config.h"
#include "../d1trn.h"

DEVILUTION_BEGIN_NAMESPACE

/*
 * In-game color translation tables.
 * 0-MAXDARKNESS: inverse brightness translations.
 * MAXDARKNESS+1: RED color translation.
 * MAXDARKNESS+2: GRAY color translation.
 * MAXDARKNESS+3: CORAL color translation.
 * MAXDARKNESS+4.. translations of unique monsters.
 */
BYTE ColorTrns[NUM_COLOR_TRNS][NUM_COLORS];

void InitLighting()
{
    //BYTE* tbl;
    //int i, j, k, l;
    //BYTE col;
    //double fs, fa;

    /*LoadFileWithMem("Levels\\TownData\\Town.TRS", ColorTrns[0]);
    LoadFileWithMem("PlrGFX\\Infra.TRN", ColorTrns[COLOR_TRN_RED]);
    LoadFileWithMem("PlrGFX\\Stone.TRN", ColorTrns[COLOR_TRN_GRAY]);
    LoadFileWithMem("PlrGFX\\Coral.TRN", ColorTrns[COLOR_TRN_CORAL]);*/
    for (int j = 0; j < NUM_COLOR_TRNS; j++) {
        for (int i = 0; i < NUM_COLORS; i++) {
            ColorTrns[j][i] = i;
        }
    }

    D1Pal pal;
    D1Trn trn;
    QString folder = Config::getAssetsFolder() + "\\";
    QString trnFilePath = folder + "PlrGFX\\Infra.TRN";
    if (trn.load(trnFilePath, &pal)) {
        for (int i = 0; i < NUM_COLORS; i++) {
            ColorTrns[COLOR_TRN_RED][i] = trn.getTranslation(i);
        }
    }
    trnFilePath = folder + "PlrGFX\\Stone.TRN";
    if (trn.load(trnFilePath, &pal)) {
        for (int i = 0; i < NUM_COLORS; i++) {
            ColorTrns[COLOR_TRN_GRAY][i] = trn.getTranslation(i);
        }
    }
}

DEVILUTION_END_NAMESPACE
