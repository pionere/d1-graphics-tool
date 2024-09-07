#include "d1hro.h"

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFontMetrics>
#include <QImage>
#include <QMessageBox>

#include "dungeon/all.h"

#include "config.h"
#include "d1cel.h"
#include "d1gfx.h"
#include "progressdialog.h"

D1Hero* D1Hero::instance()
{
    for (int pnum = 0; pnum < MAX_PLRS; pnum++) {
        if (!plr._pActive) {
            plr._pActive = TRUE;
            D1Hero* hero = new D1Hero();
            hero->pnum = pnum;
            return hero;
        }
    }
    return nullptr;
}

D1Hero::~D1Hero()
{
    plr._pActive = FALSE;
}

bool D1Hero::load(const QString &filePath, const OpenAsParam &params)
{
    // Opening CEL file and load it in RAM
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray fileData = file.readAll();

    if (fileData.size() != sizeof(PkPlayerStruct))
        return false;

    UnPackPlayer((const PkPlayerStruct*)fileData.constData(), this->pnum);

    this->filePath = filePath;
    this->modified = false;

    return true;
}

void D1Hero::create(unsigned index)
{
    _uiheroinfo selhero_heroInfo;

    //SelheroClassSelectorFocus(unsigned index)

    //selhero_heroInfo.hiIdx = MAX_CHARACTERS;
    selhero_heroInfo.hiLevel = 1;
    selhero_heroInfo.hiClass = index;
    //selhero_heroInfo.hiRank = 0;
    selhero_heroInfo.hiStrength = StrengthTbl[index];   //defaults.dsStrength;
    selhero_heroInfo.hiMagic = MagicTbl[index];         //defaults.dsMagic;
    selhero_heroInfo.hiDexterity = DexterityTbl[index]; //defaults.dsDexterity;
    selhero_heroInfo.hiVitality = VitalityTbl[index];   //defaults.dsVitality;

    selhero_heroInfo.hiName[0] = '\0';

    CreatePlayer(this->pnum, selhero_heroInfo);

    this->filePath.clear();
    this->modified = true;
}

void D1Hero::compareTo(const D1Hero *hero, QString header) const
{

}

D1Pal *D1Hero::getPalette() const
{
    return this->palette;
}

void D1Hero::setPalette(D1Pal *pal)
{
    this->palette = pal;
}

static QPainter *InvPainter = nullptr;
static D1Pal *InvPal = nullptr;
static BYTE light_trn_index = 0;
static bool gbCelTransparencyActive;

static void InvDrawSlotBack(int X, int Y, int W, int H)
{
    QImage *destImage = (QImage *)InvPainter->device();
    for (int y = Y; y < Y + H; y++) {
        for (int x = X; x < X + W; x++) {
            QColor color = destImage->pixelColor(x, y);
            for (int i = PAL16_GRAY; i < PAL16_GRAY + 15; i++) {
                if (InvPal->getColor(i) == color) {
                    color = InvPal->getColor(i - (PAL16_GRAY - PAL16_BEIGE));
                    destImage->setPixelColor(x, y, color);
                    break;
                }
            }
        }
    }
}

static void CelClippedDrawOutline(BYTE col, int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth)
{
    QImage *destImage = (QImage *)InvPainter->device();

    QColor color = InvPal->getColor(col);
    D1GfxFrame *item = pCelBuff->getFrame(nCel - 1);
    for (int y = sy - item->getHeight() + 1; y <= sy; y++) {
        for (int x = sx; x < sx + item->getWidth(); x++) {
            D1GfxPixel pixel = item->getPixel(x - sx, y - (sy - item->getHeight() + 1));
            if (!pixel.isTransparent()) {
                destImage->setPixelColor(x + 1, y, color);
                destImage->setPixelColor(x - 1, y, color);
                destImage->setPixelColor(x, y + 1, color);
                destImage->setPixelColor(x, y - 1, color);
            }
        }
    }
}

static void CelClippedDrawLightTbl(int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth, BYTE trans)
{
    QImage *destImage = (QImage *)InvPainter->device();

    D1GfxFrame *item = pCelBuff->getFrame(nCel - 1);
    for (int y = sy - item->getHeight() + 1; y <= sy; y++) {
        for (int x = sx; x < sx + item->getWidth(); x++) {
            D1GfxPixel pixel = item->getPixel(x - sx, y - (sy - item->getHeight() + 1));
            if (!pixel.isTransparent()) {
                quint8 col = pixel.getPaletteIndex();
                QColor color = InvPal->getColor(ColorTrns[trans][col]);
                destImage->setPixelColor(x, y, color);
            }
        }
    }
}

static void CelClippedDrawLightTrans(int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth)
{
    QImage *destImage = (QImage *)InvPainter->device();
    BYTE trans = light_trn_index;

    D1GfxFrame *item = pCelBuff->getFrame(nCel - 1);
    for (int y = sy - item->getHeight() + 1; y <= sy; y++) {
        for (int x = sx; x < sx + item->getWidth(); x++) {
            if ((x & 1) == (y & 1))
                continue;
            D1GfxPixel pixel = item->getPixel(x - sx, y - (sy - item->getHeight() + 1));
            if (!pixel.isTransparent()) {
                quint8 col = pixel.getPaletteIndex();
                QColor color = InvPal->getColor(ColorTrns[trans][col]);
                destImage->setPixelColor(x, y, color);
            }
        }
    }
}

static void scrollrt_draw_item(const ItemStruct* is, bool outline, int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth)
{
	BYTE col, trans;

	col = ICOL_YELLOW;
	if (is->_iMagical != ITEM_QUALITY_NORMAL) {
		col = ICOL_BLUE;
	}
	if (!is->_iStatFlag) {
		col = ICOL_RED;
	}

    if (pCelBuff != nullptr && pCelBuff->getFrameCount() > nCel) {
        if (outline) {
            CelClippedDrawOutline(col, sx, sy, pCelBuff, nCel, nWidth);
        }
        trans = col != ICOL_RED ? 0 : COLOR_TRN_RED;
        CelClippedDrawLightTbl(sx, sy, pCelBuff, nCel, nWidth, trans);
    } else {
        QString text = is->_iName;
        QFontMetrics fm(InvPainter->font());
        unsigned textWidth = fm.horizontalAdvance(text);
        if (outline) {
            col = PAL16_ORANGE + 2;
        }
        InvPainter->setPen(InvPal->getColor(col));
        InvPainter->drawText(sx + (nWidth - textWidth) / 2, sy - fm.height(), text);
    }
}

QImage D1Hero::getEquipmentImage() const
{
    constexpr int INV_WIDTH = SPANEL_WIDTH;
    constexpr int INV_HEIGHT = 178;
    constexpr int INV_ROWS = SPANEL_HEIGHT - INV_HEIGHT;
    QImage result = QImage(INV_WIDTH, INV_HEIGHT, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    QString folder = Config::getAssetsFolder() + "\\";
    // draw the inventory
    QString invFilePath = folder + "Data\\Inv\\Inv.CEL";
    if (QFile::exists(invFilePath)) {
        D1Gfx gfx;
        gfx.setPalette(this->palette);
        OpenAsParam params;
        if (D1Cel::load(gfx, invFilePath, params) && gfx.getFrameCount() != 0) {
            D1GfxFrame *inv = gfx.getFrame(0);
            int ox = (INV_WIDTH - inv->getWidth()) / 2;
            int oy = (INV_HEIGHT - (inv->getHeight() - INV_ROWS)) / 2;

            for (int y = 0; y < inv->getHeight(); y++) {
                int yy = y - oy;
                if (yy >= 0 && yy < INV_HEIGHT) {
                    for (int x = 0; x < inv->getWidth(); x++) {
                        int xx = x - ox;
                        if (xx >= 0 && xx < INV_WIDTH) {
                            D1GfxPixel pixel = inv->getPixel(x, y);
                            if (!pixel.isTransparent()) {
                                QColor color = this->palette->getColor(pixel.getPaletteIndex());
                                result.setPixelColor(xx, yy, color);
                            }
                        }
                    }
                }
            }
        }
    }
    // draw the items
    QString objFilePath = folder + "Data\\Inv\\Objcurs.CEL";
    QPainter invPainter(&result);
    invPainter.setPen(QColor(Config::getPaletteUndefinedColor()));

    InvPainter = &invPainter;
    InvPal = this->palette;

    D1Gfx *cCels = nullptr;
    if (QFile::exists(invFilePath)) {
        cCels = new D1Gfx();
        cCels->setPalette(this->palette);
        OpenAsParam params;
        if (!D1Cel::load(*cCels, objFilePath, params)) {
            delete cCels;
            cCels = nullptr;
        }
    }
    // DrawInv
	ItemStruct *is, *pi;
	int pnum, frame, frame_width, screen_x, screen_y, dx, dy, i;

	screen_x = 0;
	screen_y = 0;

    pi = nullptr; // FIXME /*pcursinvitem == ITEM_NONE ? NULL :*/ PlrItem(pnum, pcursinvitem);
	is = &plr._pInvBody[INVLOC_HEAD];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_HEAD_FIRST].X, screen_y + InvRect[SLOTXY_HEAD_LAST].Y, 2 * INV_SLOT_SIZE_PX, 2 * INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_HEAD_FIRST].X, screen_y + InvRect[SLOTXY_HEAD_LAST].Y, cCels, frame, frame_width);
	}

	is = &plr._pInvBody[INVLOC_RING_LEFT];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_RING_LEFT].X, screen_y + InvRect[SLOTXY_RING_LEFT].Y, INV_SLOT_SIZE_PX, INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_RING_LEFT].X, screen_y + InvRect[SLOTXY_RING_LEFT].Y, cCels, frame, frame_width);
	}

	is = &plr._pInvBody[INVLOC_RING_RIGHT];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_RING_RIGHT].X, screen_y + InvRect[SLOTXY_RING_RIGHT].Y, INV_SLOT_SIZE_PX, INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_RING_RIGHT].X, screen_y + InvRect[SLOTXY_RING_RIGHT].Y, cCels, frame, frame_width);
	}

	is = &plr._pInvBody[INVLOC_AMULET];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_AMULET].X, screen_y + InvRect[SLOTXY_AMULET].Y, INV_SLOT_SIZE_PX, INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_AMULET].X, screen_y + InvRect[SLOTXY_AMULET].Y, cCels, frame, frame_width);
	}

	is = &plr._pInvBody[INVLOC_HAND_LEFT];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_HAND_LEFT_FIRST].X, screen_y + InvRect[SLOTXY_HAND_LEFT_LAST].Y, 2 * INV_SLOT_SIZE_PX, 3 * INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];
		// calc item offsets for weapons smaller than 2x3 slots
		dx = 0;
		dy = 0;
		if (frame_width == INV_SLOT_SIZE_PX)
			dx += INV_SLOT_SIZE_PX / 2;
		if (InvItemHeight[frame] != (3 * INV_SLOT_SIZE_PX))
			dy -= INV_SLOT_SIZE_PX / 2;

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_HAND_LEFT_FIRST].X + dx, screen_y + InvRect[SLOTXY_HAND_LEFT_LAST].Y + dy, cCels, frame, frame_width);

		if (TWOHAND_WIELD(&plr, is)) {
				InvDrawSlotBack(screen_x + InvRect[SLOTXY_HAND_RIGHT_FIRST].X, screen_y + InvRect[SLOTXY_HAND_RIGHT_LAST].Y, 2 * INV_SLOT_SIZE_PX, 3 * INV_SLOT_SIZE_PX);
				light_trn_index = 0;
				gbCelTransparencyActive = true;

				dx = 0;
				dy = 0;
				if (frame_width == INV_SLOT_SIZE_PX)
					dx += INV_SLOT_SIZE_PX / 2;
				if (InvItemHeight[frame] != 3 * INV_SLOT_SIZE_PX)
					dy -= INV_SLOT_SIZE_PX / 2;
				CelClippedDrawLightTrans(screen_x + InvRect[SLOTXY_HAND_RIGHT_FIRST].X + dx, screen_y + InvRect[SLOTXY_HAND_RIGHT_LAST].Y + dy, cCels, frame, frame_width);
		}
	}

	is = &plr._pInvBody[INVLOC_HAND_RIGHT];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_HAND_RIGHT_FIRST].X, screen_y + InvRect[SLOTXY_HAND_RIGHT_LAST].Y, 2 * INV_SLOT_SIZE_PX, 3 * INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];
		// calc item offsets for weapons smaller than 2x3 slots
		dx = 0;
		dy = 0;
		if (frame_width == INV_SLOT_SIZE_PX)
			dx += INV_SLOT_SIZE_PX / 2;
		if (InvItemHeight[frame] != 3 * INV_SLOT_SIZE_PX)
			dy -= INV_SLOT_SIZE_PX / 2;

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_HAND_RIGHT_FIRST].X + dx, screen_y + InvRect[SLOTXY_HAND_RIGHT_LAST].Y + dy, cCels, frame, frame_width);
	}

	is = &plr._pInvBody[INVLOC_CHEST];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_CHEST_FIRST].X, screen_y + InvRect[SLOTXY_CHEST_LAST].Y, 2 * INV_SLOT_SIZE_PX, 3 * INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_CHEST_FIRST].X, screen_y + InvRect[SLOTXY_CHEST_LAST].Y, cCels, frame, frame_width);
	}
}

QString D1Hero::getFilePath() const
{
    return this->filePath;
}

void D1Hero::setFilePath(const QString &filePath)
{
    this->filePath = filePath;
    this->modified = true;
}

bool D1Hero::isModified() const
{
    return this->modified;
}

void D1Hero::setModified(bool modified)
{
    this->modified = modified;
}

const char* D1Hero::getName() const
{
    return players[this->pnum]._pName;
}

void D1Hero::setName(const QString &name)
{
    QString currName = QString(players[this->pnum]._pName);
    if (currName == name)
        return;

    memcpy(players[this->pnum]._pName, name.constData(), name.length() > lengthof(players[this->pnum]._pName) ? lengthof(players[this->pnum]._pName) : name.length());

    players[this->pnum]._pName[lengthof(players[this->pnum]._pName) - 1] = '\0';

    this->modified = true;
}

int D1Hero::getClass() const
{
    return players[this->pnum]._pClass;
}

void D1Hero::setClass(int cls)
{
    if (players[this->pnum]._pClass == cls)
        return;
    players[this->pnum]._pClass = cls;

    CalcPlrInv(this->pnum, false);

    this->modified = true;
}

int D1Hero::getLevel() const
{
    return players[this->pnum]._pLevel;
}

int D1Hero::getStatPoints() const
{
    return players[this->pnum]._pStatPts;
}

void D1Hero::setLevel(int level)
{
    int dlvl;
    dlvl = players[this->pnum]._pLevel - level;
    if (dlvl == 0)
        return;
    players[this->pnum]._pLevel = level;
    if (dlvl > 0) {
        players[this->pnum]._pStatPts += 4 * dlvl;
    } else {
        // FIXME...
    }
    players[this->pnum]._pExperience = PlrExpLvlsTbl[level - 1];

    CalcPlrInv(this->pnum, false);

    this->modified = true;
}

int D1Hero::getStrength() const
{
    return players[this->pnum]._pStrength;
}

int D1Hero::getBaseStrength() const
{
    return players[this->pnum]._pBaseStr;
}

void D1Hero::addStrength()
{
    IncreasePlrStr(this->pnum);
    this->modified = true;
}

int D1Hero::getDexterity() const
{
    return players[this->pnum]._pDexterity;
}

int D1Hero::getBaseDexterity() const
{
    return players[this->pnum]._pBaseDex;
}

void D1Hero::addDexterity()
{
    IncreasePlrDex(this->pnum);
    this->modified = true;
}

int D1Hero::getMagic() const
{
    return players[this->pnum]._pMagic;
}

int D1Hero::getBaseMagic() const
{
    return players[this->pnum]._pBaseMag;
}

void D1Hero::addMagic()
{
    IncreasePlrMag(this->pnum);
    this->modified = true;
}

int D1Hero::getVitality() const
{
    return players[this->pnum]._pVitality;
}

int D1Hero::getBaseVitality() const
{
    return players[this->pnum]._pBaseVit;
}

void D1Hero::addVitality()
{
    IncreasePlrVit(this->pnum);
    this->modified = true;
}

int D1Hero::getLife() const
{
    return players[this->pnum]._pMaxHP;
}

int D1Hero::getBaseLife() const
{
    return players[this->pnum]._pMaxHPBase;
}

int D1Hero::getMana() const
{
    return players[this->pnum]._pMaxMana;
}

int D1Hero::getBaseMana() const
{
    return players[this->pnum]._pMaxManaBase;
}

int D1Hero::getMagicResist() const
{
    return players[this->pnum]._pMagResist;
}

int D1Hero::getFireResist() const
{
    return players[this->pnum]._pFireResist;
}

int D1Hero::getLightningResist() const
{
    return players[this->pnum]._pLghtResist;
}

int D1Hero::getAcidResist() const
{
    return players[this->pnum]._pAcidResist;
}

bool D1Hero::save(const SaveAsParam &params)
{
    QString filePath = this->filePath;
    if (!params.filePath.isEmpty()) {
        filePath = params.filePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, QApplication::tr("Confirmation"), QApplication::tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    } else if (!this->isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    PkPlayerStruct pps;
    PackPlayer(&pps, this->pnum);

    // write to file
    QDataStream out(&outFile);
    bool result = out.writeRawData((char *)&pps, sizeof(PkPlayerStruct)) == sizeof(PkPlayerStruct);
    if (result) {
        this->filePath = filePath; //  D1Hero::load(gfx, filePath);
        this->modified = false;
    }

    return result;
}
