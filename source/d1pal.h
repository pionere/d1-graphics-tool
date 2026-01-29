#pragma once

#include <vector>

#include <QColor>
#include <QFile>
#include <QImage>
#include <QObject>
#include <QString>

#define D1PAL_COLORS 256
#define D1PAL_COLOR_BITS 8
#define D1PAL_SIZE_BYTES 768

enum class D1PAL_CYCLE_TYPE {
    CAVES,
    HELL,
    CRYPT,
    NEST,
};

class PaletteColor {
public:
    PaletteColor(const QColor &color, int index);
    PaletteColor(const QColor &color);
    PaletteColor(int r, int g, int b, int index);
    PaletteColor(int r, int g, int b);
    PaletteColor(const PaletteColor &o);
    ~PaletteColor() = default;

    int red() const
    {
        return rv;
    };
    int green() const
    {
        return gv;
    };
    int blue() const
    {
        return bv;
    };
    int index() const
    {
        return xv;
    };
    QColor color() const
    {
        return QColor(rv, gv, bv);
    }
    void setRed(int r) { rv = r; };
    void setGreen(int g) { gv = g; };
    void setBlue(int b) { bv = b; };
    void setIndex(int x) { xv = x; };
private:
    int rv;
    int gv;
    int bv;
    int xv;
};

class D1Pal : public QObject {
    Q_OBJECT

public:
    static constexpr const char *DEFAULT_PATH = ":/default.pal";
    static constexpr const char *EMPTY_PATH = ":/null.pal";

    D1Pal() = default;
    D1Pal(const D1Pal &opal);
    ~D1Pal() = default;

    bool load(const QString &palFilePath);
    bool save(const QString &palFilePath);

    void compareTo(const D1Pal *pal, QString header) const;

    bool reloadConfig();

    bool isModified() const;

    QString getFilePath() const;
    void setFilePath(const QString &path);
    QColor getUndefinedColor() const;
    void setUndefinedColor(QColor color);
    QColor getColor(quint8 index) const;
    void setColor(quint8 index, QColor);

    void getValidColors(std::vector<PaletteColor> &colors) const;
    void updateColors(const D1Pal &opal);
    bool genColors(const QString &imagefilePath, bool forSmk);
    bool genColors(const QImage &image, bool forSmk);
    void cycleColors(D1PAL_CYCLE_TYPE type);
    static int getCycleColors(D1PAL_CYCLE_TYPE type);
    static int getColorDist(const QColor &color, const QColor &palColor);

private:
    void loadRegularPalette(QFile &file);
    bool loadJascPalette(QFile &file);

private:
    QString palFilePath;
    bool modified;
    QColor colors[D1PAL_COLORS];
    QColor undefinedColor;
    quint8 currentCycleCounter = 3;
};
