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

    CalcPlrInv(this->pnum, false);

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
    // LogErrorF("InvDrawSlotBack %d:%d %dx%d", X, Y, W, H);
    QImage *destImage = (QImage *)InvPainter->device();
    for (int y = Y - H + 1; y <= Y; y++) {
        for (int x = X; x < X + W; x++) {
            QColor color = destImage->pixelColor(x, y);
            for (int i = PAL16_GRAY; i < PAL16_GRAY + 15; i++) {
                if (InvPal->getColor(i) == color) {
                    color = InvPal->getColor(i - (PAL16_GRAY - PAL16_BEIGE));
                    destImage->setPixelColor(x, y, color);
                    break;
                }
            }
            //QColor color = QColor(0, 0, 0);
            //destImage->setPixelColor(x, y, color);
        }
    }
    // LogErrorF("InvDrawSlotBack done");
}

static void CelClippedDrawOutline(BYTE col, int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth)
{
    // LogErrorF("CelClippedDrawOutline %d:%d idx:%d w:%d trn%d", sx, sy, nCel, nWidth, col);
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
    // LogErrorF("CelClippedDrawOutline done");
}

static void CelClippedDrawLightTbl(int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth, BYTE trans)
{
    // LogErrorF("CelClippedDrawLightTbl %d:%d idx:%d w:%d trn%d", sx, sy, nCel, nWidth, trans);
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
    // LogErrorF("CelClippedDrawLightTbl done");
}

static void CelClippedDrawLightTrans(int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth)
{
    BYTE trans = light_trn_index;
    // LogErrorF("CelClippedDrawLightTrans %d:%d idx:%d w:%d trn%d", sx, sy, nCel, nWidth, trans);
    QImage *destImage = (QImage *)InvPainter->device();

    D1GfxFrame *item = pCelBuff->getFrame(nCel - 1);
    for (int y = sy - item->getHeight() + 1; y <= sy; y++) {
        for (int x = sx; x < sx + item->getWidth(); x++) {
            if ((x & 1) == (y & 1))
                continue;
            // LogErrorF("CelClippedDrawLightTrans pixel %d:%d of %dx%d to %d:%d", x - sx, y - (sy - item->getHeight() + 1), item->getWidth(), item->getHeight(), x, y);
            D1GfxPixel pixel = item->getPixel(x - sx, y - (sy - item->getHeight() + 1));
            if (!pixel.isTransparent()) {
                quint8 col = pixel.getPaletteIndex();
                QColor color = InvPal->getColor(ColorTrns[trans][col]);
                destImage->setPixelColor(x, y, color);
            }
        }
    }
    // LogErrorF("CelClippedDrawLightTrans done");
}

static void scrollrt_draw_item(const ItemStruct* is, bool outline, int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth)
{
	BYTE col, trans;
    // dProgressErr() << QString("scrollrt_draw_item %1:%2 idx:%3 w:%4 outline%5").arg(sx).arg(sy).arg(nCel).arg(nWidth).arg(outline);
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
        QString text = ItemName(is);
        QFontMetrics fm(InvPainter->font());
        int textWidth = fm.horizontalAdvance(text);
        if (outline) {
            col = PAL16_ORANGE + 2;
        }
        InvPainter->setPen(InvPal->getColor(col));
        InvPainter->drawText(sx + (nWidth - textWidth) / 2, sy - fm.height(), text);
        // dProgressErr() << QString("scrollrt_draw_item text %1 to %2:%3 (w:%4 col%5)").arg(text).arg(sx + (nWidth - textWidth) / 2).arg(sy - fm.height()).arg(nWidth).arg(col);
    }
}

static void draw_item_placeholder(const char* name, bool outline, int sx, int sy, const D1Gfx *pCelBuff, int nCel, int nWidth)
{
    /*BYTE col = ICOL_YELLOW;
    // LogErrorF("draw_item_placeholder %s %d:%d idx:%d w:%d", name, sx, sy, nCel, nWidth);
    if (pCelBuff != nullptr && pCelBuff->getFrameCount() > nCel) {
        if (outline) {
            CelClippedDrawOutline(col, sx, sy, pCelBuff, nCel, nWidth);
        }
        CelClippedDrawLightTbl(sx, sy, pCelBuff, nCel, nWidth, COLOR_TRN_GRAY);
    } else {
        QString text = name;
        QFontMetrics fm(InvPainter->font());
        int textWidth = fm.horizontalAdvance(text);
        InvPainter->setPen(InvPal->getColor(PAL16_GRAY));
        InvPainter->drawText(sx + (nWidth - textWidth) / 2, sy - fm.height(), text);
        // LogErrorF("draw_item_placeholder text %s to %d:%d (w:%d)", name, sx + (nWidth - textWidth) / 2, sy - fm.height(), nWidth);
    }*/
}

QImage D1Hero::getEquipmentImage() const
{
    constexpr int INV_WIDTH = SPANEL_WIDTH;
    constexpr int INV_HEIGHT = 178;
    constexpr int INV_ROWS = SPANEL_HEIGHT - INV_HEIGHT;
    QImage result = QImage(INV_WIDTH, INV_HEIGHT, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    /*QString folder = Config::getAssetsFolder() + "\\";
    // draw the inventory
    QString invFilePath = folder + "Data\\Inv\\Inv.CEL";
    if (QFile::exists(invFilePath)) {
        // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("File '%1' exists").arg(invFilePath));
        D1Gfx gfx;
        gfx.setPalette(this->palette);
        OpenAsParam params = OpenAsParam();
        if (D1Cel::load(gfx, invFilePath, params) && gfx.getFrameCount() != 0) {
            D1GfxFrame *inv = gfx.getFrame(0);
            // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("File '%1' loaded %2x%3").arg(invFilePath).arg(inv->getWidth()).arg(inv->getHeight()));
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
            // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Inv copied %1:%2").arg(ox).arg(oy));
        } else {
            // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to load CEL file: %1").arg(invFilePath));
        }
    } else {
        // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to load inv %1").arg(invFilePath));
    }*/
    // draw the inventory
    D1Gfx *iCels = pInvCels;
    if (iCels != nullptr) {
        // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("File '%1' exists").arg(invFilePath));
            D1GfxFrame *inv = iCels->getFrame(0);
            // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("File '%1' loaded %2x%3").arg(invFilePath).arg(inv->getWidth()).arg(inv->getHeight()));
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

    // draw the items
    // QString objFilePath = folder + "Data\\Inv\\Objcurs.CEL";
    QPainter invPainter(&result);
    invPainter.setPen(QColor(Config::getPaletteUndefinedColor()));

    InvPainter = &invPainter;
    InvPal = this->palette;

    D1Gfx *cCels = pCursCels;
    /*if (QFile::exists(objFilePath)) {
        // dProgressErr() << QString("File '%1' exists").arg(objFilePath);
        cCels = new D1Gfx();
        cCels->setPalette(this->palette);
        OpenAsParam params = OpenAsParam();
        if (!D1Cel::load(*cCels, objFilePath, params)) {
            // dProgressErr() << QString("Failed to load CEL file: %1").arg(objFilePath);
            delete cCels;
            cCels = nullptr;
        } else {
            // dProgressErr() << QString("File '%1' loaded.").arg(objFilePath);
        }
    } else {
        // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to load obj %1").arg(objFilePath));
    }*/
    // DrawInv
	ItemStruct *is, *pi;
	int frame, frame_width, screen_x, screen_y, dx, dy;

	screen_x = 0;
	screen_y = 0;

    pi = nullptr; // FIXME /*pcursinvitem == ITEM_NONE ? NULL :*/ PlrItem(pnum, pcursinvitem);
	is = &plr._pInvBody[INVLOC_HEAD];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_HEAD_FIRST].X, screen_y + InvRect[SLOTXY_HEAD_LAST].Y, 2 * INV_SLOT_SIZE_PX, 2 * INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_HEAD_FIRST].X, screen_y + InvRect[SLOTXY_HEAD_LAST].Y, cCels, frame, frame_width);
	} else {
		frame = ICURS_HELM + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

        draw_item_placeholder("helm", pi == is, screen_x + InvRect[SLOTXY_HEAD_FIRST].X, screen_y + InvRect[SLOTXY_HEAD_LAST].Y, cCels, frame, frame_width);
    }

	is = &plr._pInvBody[INVLOC_RING_LEFT];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_RING_LEFT].X, screen_y + InvRect[SLOTXY_RING_LEFT].Y, INV_SLOT_SIZE_PX, INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_RING_LEFT].X, screen_y + InvRect[SLOTXY_RING_LEFT].Y, cCels, frame, frame_width);
	} else {
		frame = ICURS_RING + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

        draw_item_placeholder("left ring", pi == is, screen_x + InvRect[SLOTXY_RING_LEFT].X, screen_y + InvRect[SLOTXY_RING_LEFT].Y, cCels, frame, frame_width);
	}

	is = &plr._pInvBody[INVLOC_RING_RIGHT];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_RING_RIGHT].X, screen_y + InvRect[SLOTXY_RING_RIGHT].Y, INV_SLOT_SIZE_PX, INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_RING_RIGHT].X, screen_y + InvRect[SLOTXY_RING_RIGHT].Y, cCels, frame, frame_width);
	} else {
		frame = ICURS_RING + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

        draw_item_placeholder("right ring", pi == is, screen_x + InvRect[SLOTXY_RING_RIGHT].X, screen_y + InvRect[SLOTXY_RING_RIGHT].Y, cCels, frame, frame_width);
	}

	is = &plr._pInvBody[INVLOC_AMULET];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_AMULET].X, screen_y + InvRect[SLOTXY_AMULET].Y, INV_SLOT_SIZE_PX, INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_AMULET].X, screen_y + InvRect[SLOTXY_AMULET].Y, cCels, frame, frame_width);
	} else {
		frame = ICURS_AMULET + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

        draw_item_placeholder("amulet", pi == is, screen_x + InvRect[SLOTXY_AMULET].X, screen_y + InvRect[SLOTXY_AMULET].Y, cCels, frame, frame_width);
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
	} else {
        frame = 0;
        switch (plr._pClass) {
        case PC_WARRIOR:   frame = ICURS_SHORT_SWORD; break;
        case PC_ROGUE:     frame = ICURS_SHORT_BOW;   break;
        case PC_SORCERER:  frame = ICURS_SHORT_STAFF; break;
        case PC_MONK:      frame = ICURS_SHORT_STAFF; break;
        case PC_BARD:      frame = ICURS_DAGGER;      break;
        case PC_BARBARIAN: frame = ICURS_CLUB;        break;
        }
		frame += CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];
		// calc item offsets for weapons smaller than 2x3 slots
		dx = 0;
		dy = 0;
		if (frame_width == INV_SLOT_SIZE_PX)
			dx += INV_SLOT_SIZE_PX / 2;
		if (InvItemHeight[frame] != (3 * INV_SLOT_SIZE_PX))
			dy -= INV_SLOT_SIZE_PX / 2;

        draw_item_placeholder("weapon", pi == is, screen_x + InvRect[SLOTXY_HAND_LEFT_FIRST].X + dx, screen_y + InvRect[SLOTXY_HAND_LEFT_LAST].Y + dy, cCels, frame, frame_width);
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
	} else {
        is = &plr._pInvBody[INVLOC_HAND_LEFT];
        if ((is->_itype == ITYPE_NONE && (plr._pClass == PC_WARRIOR || plr._pClass == PC_BARBARIAN || plr._pClass == PC_BARD)) || !TWOHAND_WIELD(&plr, is)) {
            frame = ICURS_SMALL_SHIELD + CURSOR_FIRSTITEM;
            frame_width = InvItemWidth[frame];

            draw_item_placeholder("shield", pi == is, screen_x + InvRect[SLOTXY_HAND_RIGHT_FIRST].X, screen_y + InvRect[SLOTXY_HAND_RIGHT_LAST].Y, cCels, frame, frame_width);
        }
	}

	is = &plr._pInvBody[INVLOC_CHEST];
	if (is->_itype != ITYPE_NONE) {
		InvDrawSlotBack(screen_x + InvRect[SLOTXY_CHEST_FIRST].X, screen_y + InvRect[SLOTXY_CHEST_LAST].Y, 2 * INV_SLOT_SIZE_PX, 3 * INV_SLOT_SIZE_PX);

		frame = is->_iCurs + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

		scrollrt_draw_item(is, pi == is, screen_x + InvRect[SLOTXY_CHEST_FIRST].X, screen_y + InvRect[SLOTXY_CHEST_LAST].Y, cCels, frame, frame_width);
	} else {
		frame = ICURS_QUILTED_ARMOR + CURSOR_FIRSTITEM;
		frame_width = InvItemWidth[frame];

        draw_item_placeholder("armor", pi == is, screen_x + InvRect[SLOTXY_CHEST_FIRST].X, screen_y + InvRect[SLOTXY_CHEST_LAST].Y, cCels, frame, frame_width);
	}

    invPainter.end();

    delete cCels;

    return result;
}

const ItemStruct *D1Hero::item(int ii) const
{
    return PlrItem(this->pnum, ii);
}

void D1Hero::swapItem(int dst_ii, int src_ii)
{
    if (SwapPlrItem(this->pnum, dst_ii, src_ii)) {
        this->modified = true;
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

    int len = name.length();
    if (len > lengthof(players[this->pnum]._pName) - 1)
        len = lengthof(players[this->pnum]._pName) - 1;

    memcpy(players[this->pnum]._pName, name.toLatin1().constData(), len);

    players[this->pnum]._pName[len] = '\0';

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

    this->rebalance();

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
    dlvl = level - players[this->pnum]._pLevel;
    if (dlvl == 0)
        return;
    players[this->pnum]._pLevel = level;
    if (dlvl > 0) {
        players[this->pnum]._pStatPts += 4 * dlvl;
    } else {
        this->rebalance();
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

void D1Hero::subStrength()
{
    DecreasePlrStr(this->pnum);
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

void D1Hero::subDexterity()
{
    DecreasePlrDex(this->pnum);
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

void D1Hero::subMagic()
{
    DecreasePlrMag(this->pnum);
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

void D1Hero::subVitality()
{
    DecreasePlrVit(this->pnum);
    this->modified = true;
}

int D1Hero::getLife() const
{
    return players[this->pnum]._pMaxHP >> 6;
}

int D1Hero::getBaseLife() const
{
    return players[this->pnum]._pMaxHPBase >> 6;
}

void D1Hero::decLife()
{
    DecreasePlrMaxHp(this->pnum);
}

void D1Hero::restoreLife()
{
    RestorePlrHpVit(this->pnum);
}

int D1Hero::getMana() const
{
    return players[this->pnum]._pMaxMana >> 6;
}

int D1Hero::getBaseMana() const
{
    return players[this->pnum]._pMaxManaBase >> 6;
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

void D1Hero::rebalance()
{
    // validate the player ?
    if (plr._pClass >= NUM_CLASSES)
        plr._pClass = PC_WARRIOR;
    if (plr._pLevel < 1)
        plr._pLevel = 1;
    if (plr._pLevel > MAXCHARLEVEL)
        plr._pLevel = MAXCHARLEVEL;
    // if (plr._pStatPts < 0)
    //    plr._pStatPts = 0;
    if (plr._pBaseStr < StrengthTbl[plr._pClass])
        plr._pBaseStr = StrengthTbl[plr._pClass];
    if (plr._pBaseMag < MagicTbl[plr._pClass])
        plr._pBaseMag = MagicTbl[plr._pClass];
    if (plr._pBaseDex < DexterityTbl[plr._pClass])
        plr._pBaseDex = DexterityTbl[plr._pClass];
    if (plr._pBaseVit < VitalityTbl[plr._pClass])
        plr._pBaseVit = VitalityTbl[plr._pClass];

    int tmp;
    switch (plr._pClass) {
    case PC_WARRIOR:
        tmp = 5 * ((plr._pBaseStr - StrengthTbl[PC_WARRIOR]) / 5) + StrengthTbl[PC_WARRIOR];
        plr._pBaseStr = tmp + (tmp == plr._pBaseStr ? 0 : 2);
        tmp = 3 * ((plr._pBaseDex - DexterityTbl[PC_WARRIOR]) / 3) + DexterityTbl[PC_WARRIOR];
        plr._pBaseDex = tmp + (tmp == plr._pBaseDex ? 0 : 1);
        plr._pBaseVit = 2 * ((plr._pBaseVit - VitalityTbl[PC_WARRIOR]) / 2) + VitalityTbl[PC_WARRIOR];
        break;
    case PC_ROGUE:
        plr._pBaseMag = 2 * ((plr._pBaseMag - MagicTbl[PC_ROGUE]) / 2) + MagicTbl[PC_ROGUE];
        plr._pBaseDex = 3 * ((plr._pBaseDex - DexterityTbl[PC_ROGUE]) / 3) + DexterityTbl[PC_ROGUE];
        break;
    case PC_SORCERER:
        plr._pBaseMag = 3 * ((plr._pBaseMag - MagicTbl[PC_SORCERER]) / 3) + MagicTbl[PC_SORCERER];
        tmp = 3 * ((plr._pBaseDex - DexterityTbl[PC_SORCERER]) / 3) + DexterityTbl[PC_SORCERER];
        plr._pBaseDex = tmp + (tmp == plr._pBaseDex ? 0 : 1);
        tmp = 3 * ((plr._pBaseVit - VitalityTbl[PC_SORCERER]) / 3) + VitalityTbl[PC_SORCERER];
        plr._pBaseVit = tmp + (tmp == plr._pBaseVit ? 0 : 1);
        break;
#ifdef HELLFIRE
    case PC_MONK:
        plr._pBaseStr = 2 * ((plr._pBaseStr - StrengthTbl[PC_MONK]) / 2) + StrengthTbl[PC_MONK];
        tmp = 3 * ((plr._pBaseMag - MagicTbl[PC_MONK]) / 3) + MagicTbl[PC_MONK];
        plr._pBaseMag = tmp + (tmp == plr._pBaseMag ? 0 : 1);
        plr._pBaseDex = 2 * ((plr._pBaseDex - DexterityTbl[PC_MONK]) / 2) + DexterityTbl[PC_MONK];
        tmp = 3 * ((plr._pBaseVit - VitalityTbl[PC_MONK]) / 3) + VitalityTbl[PC_MONK];
        plr._pBaseVit = tmp + (tmp == plr._pBaseVit ? 0 : 1);
        break;
    case PC_BARD:
        tmp = 3 * ((plr._pBaseMag - MagicTbl[PC_BARD]) / 3) + MagicTbl[PC_BARD];
        plr._pBaseMag = tmp + (tmp == plr._pBaseMag ? 0 : 1);
        plr._pBaseDex = 3 * ((plr._pBaseDex - DexterityTbl[PC_BARD]) / 3) + DexterityTbl[PC_BARD];
        tmp = 3 * ((plr._pBaseVit - VitalityTbl[PC_BARD]) / 3) + VitalityTbl[PC_BARD];
        plr._pBaseVit = tmp + (tmp == plr._pBaseVit ? 0 : 1);
        break;
    case PC_BARBARIAN:
        plr._pBaseStr = 3 * ((plr._pBaseStr - StrengthTbl[PC_BARBARIAN]) / 3) + StrengthTbl[PC_BARBARIAN];
        plr._pBaseVit = 2 * ((plr._pBaseVit - VitalityTbl[PC_BARBARIAN]) / 2) + VitalityTbl[PC_BARBARIAN];
        break;
#endif
    default:
        ASSUME_UNREACHABLE
            break;
    }

    // if (change) {
        // RestorePlrHpVit
        plr._pMaxHPBase = plr._pBaseVit << (6 + 1);
        // RestorePlrHpMana
        plr._pMaxManaBase = plr._pBaseMag << (6 + 1);
    // }

    while (true) {
        int remStatPts = (plr._pLevel - 1) * 4;

        if (plr._pStatPts > remStatPts) {
            plr._pStatPts = remStatPts;
        }
        remStatPts -= plr._pStatPts;

        // LogErrorF("PlrStats: [str%d mag%d dex%d vit%d] vs base[str%d mag%d dex%d vit%d] (bonus%d)", plr._pBaseStr, plr._pBaseMag, plr._pBaseDex, plr._pBaseVit,
        //     StrengthTbl[plr._pClass], MagicTbl[plr._pClass], DexterityTbl[plr._pClass], VitalityTbl[plr._pClass], plr._pStatPts);

        int usedStatPts[4];
        switch (plr._pClass) {
        case PC_WARRIOR:
            usedStatPts[0] = ((plr._pBaseStr - StrengthTbl[PC_WARRIOR]) * 2 + 4) / 5;
            usedStatPts[1] = plr._pBaseMag - MagicTbl[PC_WARRIOR];
            usedStatPts[2] = ((plr._pBaseDex - DexterityTbl[PC_WARRIOR]) * 2 + 2) / 3;
            usedStatPts[3] = (plr._pBaseVit - VitalityTbl[PC_WARRIOR]) / 2;
            break;
        case PC_ROGUE:
            usedStatPts[0] = plr._pBaseStr - StrengthTbl[PC_ROGUE];
            usedStatPts[1] = (plr._pBaseMag - MagicTbl[PC_ROGUE]) / 2;
            usedStatPts[2] = (plr._pBaseDex - DexterityTbl[PC_ROGUE]) / 3;
            usedStatPts[3] = plr._pBaseVit - VitalityTbl[PC_ROGUE];
            break;
        case PC_SORCERER:
            usedStatPts[0] = plr._pBaseStr - StrengthTbl[PC_SORCERER];
            usedStatPts[1] = (plr._pBaseMag - MagicTbl[PC_SORCERER]) / 3;
            usedStatPts[2] = ((plr._pBaseDex - DexterityTbl[PC_SORCERER]) * 2 + 2) / 3;
            usedStatPts[3] = ((plr._pBaseVit - VitalityTbl[PC_SORCERER]) * 2 + 2) / 3;
            break;
#ifdef HELLFIRE
        case PC_MONK:
            usedStatPts[0] = (plr._pBaseStr - StrengthTbl[PC_MONK]) / 2;
            usedStatPts[1] = ((plr._pBaseMag - MagicTbl[PC_MONK]) * 2 + 2) / 3;
            usedStatPts[2] = (plr._pBaseDex - DexterityTbl[PC_MONK]) / 2;
            usedStatPts[3] = ((plr._pBaseVit - VitalityTbl[PC_MONK]) * 2 + 2) / 3;
            break;
        case PC_BARD:
            usedStatPts[0] = plr._pBaseStr - StrengthTbl[PC_BARD];
            usedStatPts[1] = ((plr._pBaseMag - MagicTbl[PC_BARD]) * 2 + 2) / 3;
            usedStatPts[2] = (plr._pBaseDex - DexterityTbl[PC_BARD]) / 3;
            usedStatPts[3] = ((plr._pBaseVit - VitalityTbl[PC_BARD]) * 2 + 2) / 3;
            break;
        case PC_BARBARIAN:
            usedStatPts[0] = (plr._pBaseStr - StrengthTbl[PC_BARBARIAN]) / 3;
            usedStatPts[1] = plr._pBaseMag - MagicTbl[PC_BARBARIAN];
            usedStatPts[2] = plr._pBaseDex - DexterityTbl[PC_BARBARIAN];
            usedStatPts[3] = (plr._pBaseVit - VitalityTbl[PC_BARBARIAN]) / 2;
            break;
#endif
        default:
            ASSUME_UNREACHABLE
                break;
        }

        int totalUsedStatPts = usedStatPts[0] + usedStatPts[1] + usedStatPts[2] + usedStatPts[3];
        // LogErrorF("UsedStats: %d [str%d mag%d dex%d vit%d] vs %d (bonus%d)", totalUsedStatPts, usedStatPts[0], usedStatPts[1], usedStatPts[2], usedStatPts[3], remStatPts, plr._pStatPts);
        if (totalUsedStatPts <= remStatPts) {
            plr._pStatPts += remStatPts - totalUsedStatPts;
            break;
        }

        int mostUsedIdx = 0;
        int mostUsedVal = usedStatPts[0];
        for (int i = 1; i < 4; i++) {
            if (usedStatPts[i] > mostUsedVal) {
                mostUsedIdx = i;
                mostUsedVal = usedStatPts[i];
            }
        }
        switch (mostUsedIdx) {
        case 0: DecreasePlrStr(pnum); break;
        case 1: DecreasePlrMag(pnum); break;
        case 2: DecreasePlrDex(pnum); break;
        case 3: DecreasePlrVit(pnum); break;
        }
        plr._pStatPts--;
        // LogErrorF("mostUsedIdx: %d -> (bonus%d)", mostUsedIdx, plr._pStatPts);
    }
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
