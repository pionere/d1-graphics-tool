#pragma once

#include <vector>

#include <QImage>

#include "d1pal.h"
#include "openasdialog.h"
#include "saveasdialog.h"

enum class D1TBL_TYPE {
    V1_DARK,
    V1_DIST,
    V1_UNDEF,
};

class D1Tbl : public QObject {
    Q_OBJECT

public:
    D1Tbl() = default;
    ~D1Tbl() = default;

    bool load(const QString &filePath, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    static int getTableImageWidth();
    static int getTableImageHeight();
    QImage getTableImage(const D1Pal *pal, int radius, int xoff, int yoff, int dunType, int color);
    static int getTableValueAt(int x, int y);
    static int getLightImageWidth();
    static int getLightImageHeight();
    static QImage getLightImage(const D1Pal *pal, int color);
    static int getLightValueAt(const D1Pal *pal, int x, int color);
    static int getLumImageWidth();
    static int getLumImageHeight();
    static QImage getLumImage(const D1Pal *pal, int color);
    static int getLumValueAt(const D1Pal *pal, int x, int color);
    static int getDarkImageWidth();
    static int getDarkImageHeight();
    static QImage getDarkImage(int radius);
    static int getDarkValueAt(int x, int radius);
    int setDarkValueAt(int x, int radius, int value);

    D1TBL_TYPE getType() const;
    void setType(D1TBL_TYPE type);
    QString getFilePath() const;
    bool isModified() const;
    void setModified();

private:
    D1TBL_TYPE type = D1TBL_TYPE::V1_UNDEF;
    QString tblFilePath;
    bool modified;
    int lastDunType = -1;
};
