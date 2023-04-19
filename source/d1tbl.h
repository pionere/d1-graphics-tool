#pragma once

#include <vector>

#include <QImage>

#include "d1pal.h"
#include "saveasdialog.h"

enum class D1TBL_TYPE {
    V1_DARK,
    V1_DIST,
};

class D1Tbl : public QObject {
    Q_OBJECT

public:
    D1Tbl() = default;
    ~D1Tbl() = default;

    bool load(const QString &filePath);
    bool save(const SaveAsParam &params);

    static int getTableImageWidth();
    static int getTableImageHeight();
    QImage getTableImage(int radius, int dunType, int color) const;
    int getTableValueAt(int x, int y) const;
    static int getLightImageWidth();
    static int getLightImageHeight();
    QImage getLightImage(int color) const;
    static int getDarkImageWidth();
    static int getDarkImageHeight();
    QImage getDarkImage(int radius) const;
    int getDarkValueAt(int x, int radius) const;

    QString getFilePath() const;
    bool isModified() const;
    void setModified();
    void setPalette(D1Pal *pal);

private:
    D1TBL_TYPE type = D1TBL_TYPE::V1_DARK;
    QString tblFilePath;
    bool modified;
    D1Pal *pal = nullptr;
};
