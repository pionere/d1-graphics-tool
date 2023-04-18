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

    QImage getTableImage(int radius, int dunType, int color) const;

    QString getFilePath() const;
    bool isModified() const;
    void setModified();
    void setPalette(D1Pal *pal);

private:
    D1TBL_TYPE type = D1TBL_TYPE::V1_DARK;
    QString tblFilePath;
    bool modified;
    D1Pal *palette = nullptr;
};
