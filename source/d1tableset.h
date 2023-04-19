#pragma once

#include "d1pal.h"
#include "d1tbl.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D1Tableset {
public:
    D1Tableset();
    ~D1Tableset();

    bool load(const QString &tblfilePath1, const QString &tblfilePath2, const OpenAsParam &params);
    void save(const SaveAsParam &params);

    D1Tbl *distTbl;
    D1Tbl *darkTbl;
};
