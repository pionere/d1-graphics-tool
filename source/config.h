#pragma once

#include <QJsonObject>
#include <QString>

class Config {
private:
    static constexpr const char *FILE_PATH = "/D1HeroPlanner.config.json";
    static constexpr const char *DEFAULT_LOCALE = "en_US";
    static constexpr const char *DEFAULT_PAL_UNDEFINED_COLOR = "#ff00ff";        // magenta
    static constexpr const char *DEFAULT_PAL_SELECTION_BORDER_COLOR = "#ff0000"; // red
    static constexpr const char *DEFAULT_GRAPHICS_BACKGROUND_COLOR = "#ffffff";  // white
    static constexpr const char *DEFAULT_GRAPHICS_TRANSPARENT_COLOR = "#808080"; // gray

    static constexpr const char *CFG_LOCALE = "Locale";
    static constexpr const char *CFG_LAST_FILE_PATH = "LastFilePath";
    static constexpr const char *CFG_ASSETS_FOLDER = "AssetsFolder";
    static constexpr const char *CFG_PAL_UNDEFINED_COLOR = "PaletteUndefinedColor";
    static constexpr const char *CFG_PAL_SELECTION_BORDER_COLOR = "PaletteSelectionBorderColor";
    static constexpr const char *CFG_GRAPHICS_BACKGROUND_COLOR = "GraphicsBackgroundColor";
    static constexpr const char *CFG_GRAPHICS_TRANSPARENT_COLOR = "GraphicsTransparentColor";

    static QString getJsonFilePath();
    static QJsonValue value(const QString &name);
    static void insert(const QString &key, const QJsonValue &value);

public:
    static void loadConfiguration();
    static bool storeConfiguration();

    static QString getLocale()
    {
        return Config::value(Config::CFG_LOCALE).toString();
    };
    static void setLocale(const QString &locale)
    {
        Config::insert(Config::CFG_LOCALE, locale);
    };
    static QString getLastFilePath()
    {
        return Config::value(Config::CFG_LAST_FILE_PATH).toString();
    };
    static void setLastFilePath(const QString &path)
    {
        Config::insert(Config::CFG_LAST_FILE_PATH, path);
    };
    static QString getAssetsFolder()
    {
        return Config::value(Config::CFG_ASSETS_FOLDER).toString();
    };
    static void setAssetsFolder(const QString &path)
    {
        Config::insert(Config::CFG_ASSETS_FOLDER, path);
    };
    static QString getPaletteUndefinedColor()
    {
        return Config::value(Config::CFG_PAL_UNDEFINED_COLOR).toString();
    };
    static void setPaletteUndefinedColor(QString colorName)
    {
        Config::insert(Config::CFG_PAL_UNDEFINED_COLOR, colorName);
    };
    static QString getPaletteSelectionBorderColor()
    {
        return Config::value(Config::CFG_PAL_SELECTION_BORDER_COLOR).toString();
    };
    static void setPaletteSelectionBorderColor(QString colorName)
    {
        Config::insert(Config::CFG_PAL_SELECTION_BORDER_COLOR, colorName);
    };
    static QString getGraphicsBackgroundColor()
    {
        return Config::value(Config::CFG_GRAPHICS_BACKGROUND_COLOR).toString();
    };
    static void setGraphicsBackgroundColor(QString colorName)
    {
        Config::insert(Config::CFG_GRAPHICS_BACKGROUND_COLOR, colorName);
    };
    static QString getGraphicsTransparentColor()
    {
        return Config::value(Config::CFG_GRAPHICS_TRANSPARENT_COLOR).toString();
    };
    static void setGraphicsTransparentColor(QString colorName)
    {
        Config::insert(Config::CFG_GRAPHICS_TRANSPARENT_COLOR, colorName);
    };
};
