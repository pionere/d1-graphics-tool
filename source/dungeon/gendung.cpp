/**
 * @file gendung.cpp
 *
 * Implementation of general dungeon generation code.
 */
#include "all.h"

#include <QApplication>
#include <QString>

#include "../progressdialog.h"

DEVILUTION_BEGIN_NAMESPACE

/** The difficuly level of the current game (_difficulty) */
int gnDifficulty;
/** Contains the data of the active dungeon level. */
LevelStruct currLvl;

DEVILUTION_END_NAMESPACE
