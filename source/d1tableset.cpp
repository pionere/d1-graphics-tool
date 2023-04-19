#include "d1tableset.h"

#include <QApplication>

#include "progressdialog.h"

#include "dungeon/all.h"

D1Tableset::D1Tableset()
{
    this->distTbl = new D1Tbl();
    this->darkTbl = new D1Tbl();
}

D1Tableset::~D1Tableset()
{
    delete distTbl;
    delete darkTbl;
}

bool D1Tableset::load(const QString &tblfilePath1, const QString &tblfilePath2, const OpenAsParam &params)
{
    dProgress() << QString("Darktabla at load 0: %1").arg(darkTable[0][10]);
    if (!this->distTbl->load(tblfilePath1, params)) {
        dProgressErr() << QApplication::tr("Failed loading TBL file: %1.").arg(QDir::toNativeSeparators(tblfilePath1));
        return false;
    }
    dProgress() << QString("Darktabla at load 1: %1").arg(darkTable[0][10]);
    if (!this->darkTbl->load(tblfilePath2, params)) {
        dProgressErr() << QApplication::tr("Failed loading TBL file: %1.").arg(QDir::toNativeSeparators(tblfilePath2));
        return false;
    }
    dProgress() << QString("Darktabla at load 2: %1").arg(darkTable[0][10]);

    D1TBL_TYPE darkType = this->darkTbl->getType();
    D1TBL_TYPE distType = this->distTbl->getType();
    if (darkType == distType) {
        if (darkType != D1TBL_TYPE::V1_UNDEF) {
            dProgressErr() << QApplication::tr("The table files have the same type of content.");
            return false;
        }
    } else {
        if (darkType == D1TBL_TYPE::V1_DIST || distType == D1TBL_TYPE::V1_DARK) {
            std::swap(this->darkTbl, this->distTbl);
            std::swap(darkType, distType);
        }
    }
    if (darkType == D1TBL_TYPE::V1_UNDEF) {
        this->darkTbl->setType(D1TBL_TYPE::V1_DARK);
    }
    if (distType == D1TBL_TYPE::V1_UNDEF) {
        this->distTbl->setType(D1TBL_TYPE::V1_DIST);
    }
    dProgress() << QString("Darktabla at load end: %1").arg(darkTable[0][10]);
    return true;
}

void D1Tableset::save(const SaveAsParam &params)
{
    this->distTbl->save(params);
    this->darkTbl->save(params);
}
