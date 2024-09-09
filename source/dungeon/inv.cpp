/**
 * @file inv.cpp
 *
 * Implementation of player inventory.
 */
#include "all.h"

#include <QFile>
#include <QString>

#include "../config.h"
#include "../d1cel.h"
#include "../d1gfx.h"
#include "../openasdialog.h"

DEVILUTION_BEGIN_NAMESPACE

D1Gfx *pInvCels;

/**
 * Maps from inventory slot to screen position. The inventory slots are
 * arranged as follows:
 *                          00 01
 *                          02 03   06
 *              07 08       19 20       13 14
 *              09 10       21 22       15 16
 *              11 12       23 24       17 18
 *                 04                   05
 *              25 26 27 28 29 30 31 32 33 34
 *              35 36 37 38 39 40 41 42 43 44
 *              45 46 47 48 49 50 51 52 53 54
 *              55 56 57 58 59 60 61 62 63 64
 * 65 66 67 68 69 70 71 72
 */
const InvXY InvRect[NUM_XY_SLOTS] = {
	// clang-format off
	//  X,   Y
	{ 120,                    30 },                       // helmet
	{ 120 + INV_SLOT_SIZE_PX, 30 },                       // helmet
	{ 120,                    30 +    INV_SLOT_SIZE_PX }, // helmet
	{ 120 + INV_SLOT_SIZE_PX, 30 +    INV_SLOT_SIZE_PX }, // helmet
	{  60, 172 }, // left ring
	{ 205, 172 }, // right ring
	{ 186,  44 }, // amulet
	{  47,                    82 },                      // left hand
	{  47 + INV_SLOT_SIZE_PX, 82 },                      // left hand
	{  47,                    82 +   INV_SLOT_SIZE_PX }, // left hand
	{  47 + INV_SLOT_SIZE_PX, 82 +   INV_SLOT_SIZE_PX }, // left hand
	{  47,                    82 + 2*INV_SLOT_SIZE_PX }, // left hand
	{  47 + INV_SLOT_SIZE_PX, 82 + 2*INV_SLOT_SIZE_PX }, // left hand
	{ 192,                    82 },                      // right hand
	{ 192 + INV_SLOT_SIZE_PX, 82 },                      // right hand
	{ 192,                    82 +   INV_SLOT_SIZE_PX }, // right hand
	{ 192 + INV_SLOT_SIZE_PX, 82 +   INV_SLOT_SIZE_PX }, // right hand
	{ 192,                    82 + 2*INV_SLOT_SIZE_PX }, // right hand
	{ 192 + INV_SLOT_SIZE_PX, 82 + 2*INV_SLOT_SIZE_PX }, // right hand
	{ 120,                    94 },                      // chest
	{ 120 + INV_SLOT_SIZE_PX, 94 },                      // chest
	{ 120,                    94 +   INV_SLOT_SIZE_PX }, // chest
	{ 120 + INV_SLOT_SIZE_PX, 94 +   INV_SLOT_SIZE_PX }, // chest
	{ 120,                    94 + 2*INV_SLOT_SIZE_PX }, // chest
	{ 120 + INV_SLOT_SIZE_PX, 94 + 2*INV_SLOT_SIZE_PX }, // chest
	{  2 + 0 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 1 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 2 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 3 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 4 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 5 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 6 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 7 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 8 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 9 * (INV_SLOT_SIZE_PX + 1), 206 }, // inv row 1
	{  2 + 0 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 1 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 2 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 3 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 4 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 5 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 6 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 7 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 8 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 9 * (INV_SLOT_SIZE_PX + 1), 206 + 1 * (INV_SLOT_SIZE_PX + 1) }, // inv row 2
	{  2 + 0 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 1 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 2 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 3 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 4 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 5 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 6 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 7 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 8 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 9 * (INV_SLOT_SIZE_PX + 1), 206 + 2 * (INV_SLOT_SIZE_PX + 1) }, // inv row 3
	{  2 + 0 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 1 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 2 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 3 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 4 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 5 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 6 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 7 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 8 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{  2 + 9 * (INV_SLOT_SIZE_PX + 1), 206 + 3 * (INV_SLOT_SIZE_PX + 1) }, // inv row 4
	{ 1                   ,  1 * (INV_SLOT_SIZE_PX + 1) }, // belt column 1
	{ 1                   ,  2 * (INV_SLOT_SIZE_PX + 1) }, // belt column 1
	{ 1                   ,  3 * (INV_SLOT_SIZE_PX + 1) }, // belt column 1
	{ 1                   ,  4 * (INV_SLOT_SIZE_PX + 1) }, // belt column 1
	{ 2 + INV_SLOT_SIZE_PX,  1 * (INV_SLOT_SIZE_PX + 1) }, // belt column 2
	{ 2 + INV_SLOT_SIZE_PX,  2 * (INV_SLOT_SIZE_PX + 1) }, // belt column 2
	{ 2 + INV_SLOT_SIZE_PX,  3 * (INV_SLOT_SIZE_PX + 1) }, // belt column 2
	{ 2 + INV_SLOT_SIZE_PX,  4 * (INV_SLOT_SIZE_PX + 1) }  // belt column 2
	// clang-format on
};

/* InvSlotXY to InvSlot map. */
const BYTE InvSlotTbl[] = {
	// clang-format off
	SLOT_HEAD,
	SLOT_HEAD,
	SLOT_HEAD,
	SLOT_HEAD,
	SLOT_RING_LEFT,
	SLOT_RING_RIGHT,
	SLOT_AMULET,
	SLOT_HAND_LEFT,
	SLOT_HAND_LEFT,
	SLOT_HAND_LEFT,
	SLOT_HAND_LEFT,
	SLOT_HAND_LEFT,
	SLOT_HAND_LEFT,
	SLOT_HAND_RIGHT,
	SLOT_HAND_RIGHT,
	SLOT_HAND_RIGHT,
	SLOT_HAND_RIGHT,
	SLOT_HAND_RIGHT,
	SLOT_HAND_RIGHT,
	SLOT_CHEST,
	SLOT_CHEST,
	SLOT_CHEST,
	SLOT_CHEST,
	SLOT_CHEST,
	SLOT_CHEST,
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 1
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 2
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 3
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_STORAGE, // inv row 4
	SLOT_BELT,
	SLOT_BELT,
	SLOT_BELT,
	SLOT_BELT,
	SLOT_BELT,
	SLOT_BELT,
	SLOT_BELT,
	SLOT_BELT
	// clang-format on
};

void InitInv(D1Pal *pal)
{
    delete pInvCels;
    pInvCels = nullptr;

    QString folder = Config::getAssetsFolder() + "\\";
    QString invFilePath = folder + "Data\\Inv\\Inv.CEL";
    if (QFile::exists(invFilePath)) {
        // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("File '%1' exists").arg(invFilePath));
        pInvCels = new D1Gfx();
        pInvCels->setPalette(pal);
        OpenAsParam params = OpenAsParam();
        if (!D1Cel::load(*pInvCels, invFilePath, params) || pInvCels->getFrameCount() == 0) {
            // dProgressErr() << QString("Failed to load CEL file: %1").arg(objFilePath);
            delete pInvCels;
            pInvCels = nullptr;
        } else {
            // dProgressErr() << QString("File '%1' loaded.").arg(objFilePath);
        }
    } else {
        // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to load inv %1").arg(invFilePath));
    }
}

void CalculateGold(int pnum)
{
	ItemStruct* pi;
	int i, gold;

	gold = 0;
	pi = plr._pInvList;
	for (i = NUM_INV_GRID_ELEM; i > 0; i--, pi++) {
		if (pi->_itype == ITYPE_GOLD)
			gold += pi->_ivalue;
	}

	plr._pGold = gold;
}

DEVILUTION_END_NAMESPACE
