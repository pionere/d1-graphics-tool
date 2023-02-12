#pragma once

#include "d1pal.h"

#define D1TRN_TRANSLATIONS 256

class D1Trn : public QObject {
    Q_OBJECT

public:
    static constexpr const char *IDENTITY_PATH = ":/null.trn";

    D1Trn() = default;
    ~D1Trn() = default;

    bool load(const QString &trnFilePath, D1Pal *pal);
    bool save(const QString &trnFilePath);

    bool isModified() const;

    void refreshResultingPalette();
    QColor getResultingColor(quint8 index) const;

    QString getFilePath() const;
    void setFilePath(const QString &path);
    quint8 getTranslation(quint8 index) const;
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
