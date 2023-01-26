#pragma once

#include "d1pal.h"

#define D1TRN_TRANSLATIONS 256

class D1Trn : public QObject {
    Q_OBJECT

public:
    static constexpr const char *IDENTITY_PATH = ":/null.trn";
    static constexpr const char *IDENTITY_NAME = "_null.trn";

    D1Trn() = default;
    ~D1Trn() = default;

    bool load(QString filePath, D1Pal *pal);
    bool save(QString filePath);

    bool isModified() const;

    void refreshResultingPalette();
    QColor getResultingColor(quint8 index);

    QString getFilePath();
    quint8 getTranslation(quint8 index);
    void setTranslation(quint8 index, quint8 color);
    void setPalette(D1Pal *pal);
    D1Pal *getResultingPalette();

private:
    QString trnFilePath;
    bool modified;
    quint8 translations[D1TRN_TRANSLATIONS];
    D1Pal *palette;
    D1Pal resultingPalette;
};
