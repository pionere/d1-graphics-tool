#include "d1trs.h"

#include <QApplication>
#include <QChar>
#include <QDir>
#include <QFile>
#include <QMessageBox>

#include "trngeneratedialog.h"
#include "dungeon/all.h"

bool D1Trs::load(const QString &filePath, D1Pal *pal, std::vector<D1Trn *> &trns)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    size_t fileSize = file.size();
    if ((fileSize % D1TRN_TRANSLATIONS) != 0)
        return false;

    const QByteArray fileData = file.readAll();
    // Read SLA binary data
    QDataStream in(fileData);

    unsigned numTrns = fileSize / D1TRN_TRANSLATIONS;
    for (unsigned i = 0; i < numTrns; i++) {
        D1Trn *trn = new D1Trn();
        quint8 readByte;
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            in >> readByte;
            trn->setTranslation(n, readByte);
        }
        QString trnFilePath = filePath;
        trnFilePath.insert(filePath.length() - 4, QString::number(i));
        trn->setFilePath(trnFilePath);
        trn->setPalette(pal);
        trn->refreshResultingPalette();
        trns.push_back(trn);
    }
    return true;
}

bool D1Trs::save(const QString &filePath, const std::vector<D1Trn *> &trns)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    // write to file
    QDataStream out(&outFile);

    for (const D1Trn *trn : trns) {
        quint8 writeByte;
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            writeByte = trn->getTranslation(n);
            out << writeByte;
        }
    }

    return true;
}

static void MakeLightTable(const GenerateTrnParam &params)
{
	unsigned i, j, k, shade;
	BYTE col, max;
	BYTE* tbl;

	tbl = ColorTrns[0];
	for (i = 0; i < MAXDARKNESS; i++) {
		static_assert(MAXDARKNESS == 15, "Shade calculation requires MAXDARKNESS to be 15.");
		// shade calculation is simplified by using a fix MAXDARKNESS value.
		// otherwise the correct calculation would be as follows:
		//	shade = (i * 15 + (MAXDARKNESS + 1) / 2) / MAXDARKNESS;
		shade = i;
		// light trns of the level palette
		*tbl++ = 0;
		for (j = 0; j < 8; j++) {
			col = 16 * j + shade;
			max = 16 * j + 15;
			for (k = 0; k < 16; k++) {
				if (k != 0 || j != 0) {
					*tbl++ = col;
				}
				if (col < max) {
					col++;
				} else {
					max = 0;
					col = 0;
				}
			}
		}
		// light trns of the standard palette
		//  (PAL8_BLUE, PAL8_RED, PAL8_YELLOW, PAL8_ORANGE)
		for (j = 16; j < 20; j++) {
			col = 8 * j + (shade >> 1);
			max = 8 * j + 7;
			for (k = 0; k < 8; k++) {
				*tbl++ = col;
				if (col < max) {
					col++;
				} else {
					max = 0;
					col = 0;
				}
			}
		}
		//  (PAL16_BEIGE, PAL16_BLUE, PAL16_YELLOW, PAL16_ORANGE, PAL16_RED, PAL16_GRAY)
		for (j = 10; j < 16; j++) {
			col = 16 * j + shade;
			max = 16 * j + 15;
			if (max == 255) {
				max = 254;
			}
			for (k = 0; k < 16; k++) {
				*tbl++ = col;
				if (col < max) {
					col++;
				} else {
					max = 0;
					col = 0;
				}
			}
		}
	}

	// assert(tbl == ColorTrns[MAXDARKNESS]);
	tbl = ColorTrns[0];
	memset(ColorTrns[MAXDARKNESS], 0, sizeof(ColorTrns[MAXDARKNESS]));

	if (currLvl._dType == DTYPE_HELL) {
		for (i = 0; i <= MAXDARKNESS; i++) {
			shade = i;
			col = 1;
			*tbl++ = 0;
			for (k = 1; k < 16; k++) {
				*tbl++ = col;
				if (shade > 0) {
					shade--;
				} else {
					col++;
				}
			}
			shade = i;
			col = 16 * 1 + shade;
			max = 16 * 1 + 15;
			for (k = 0; k < 16; k++) {
				*tbl++ = col;
				if (col < max) {
					col++;
				} else {
					max = 1;
					col = 1;
				}
			}
			tbl += NUM_COLORS - 32;
		}
#ifdef HELLFIRE
	} else if (currLvl._dType == DTYPE_CAVES || currLvl._dType == DTYPE_CRYPT) {
#else
	} else if (currLvl._dType == DTYPE_CAVES) {
#endif
		for (i = 0; i <= MAXDARKNESS; i++) {
			*tbl++ = 0;
			for (j = 1; j < 32; j++)
				*tbl++ = j;
			tbl += NUM_COLORS - 32;
		}
#ifdef HELLFIRE
	} else if (currLvl._dType == DTYPE_NEST) {
		for (i = 0; i <= MAXDARKNESS; i++) {
			*tbl++ = 0;
			for (j = 1; j < 16; j++)
				*tbl++ = j;
			tbl += NUM_COLORS - 16;
		}
#endif
	}
}

void D1Trs::generateLightTranslations(const GenerateTrnParam &params, D1Pal *pal, std::vector<D1Trn *> &trns)
{
    MakeLightTable(params);

    QString filePath = QApplication::tr("Light%1.trn");
    for (int i = 0; i <= MAXDARKNESS; i++) {
        D1Trn *trn = new D1Trn();
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            trn->setTranslation(n, ColorTrns[i][n]);
        }
        QString trnFilePath = filePath.arg(i, 2, 10, QChar('0'));
        trn->setFilePath(trnFilePath);
        trn->setPalette(pal);
        trn->refreshResultingPalette();
        trns.push_back(trn);
    }
}
