#include "config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

static QJsonObject theConfig;

QString Config::getJsonFilePath()
{
    QString dirPath = QCoreApplication::applicationDirPath();
    if (!QFileInfo(dirPath).isWritable()) {
        dirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(dirPath);
    }
    return dirPath + Config::FILE_PATH;
}

void Config::loadConfiguration()
{
    QString jsonFilePath = Config::getJsonFilePath();
    bool configurationModified = false;

    // If configuration file exists load it otherwise create it
    if (QFile::exists(jsonFilePath)) {
        QFile loadJson(jsonFilePath);
        loadJson.open(QIODevice::ReadOnly | QIODevice::Text);
        QJsonDocument loadJsonDoc = QJsonDocument::fromJson(loadJson.readAll());
        theConfig = loadJsonDoc.object();
        loadJson.close();
    }

    if (!theConfig.contains(Config::CFG_LOCALE)) {
        Config::setLocale(Config::DEFAULT_LOCALE);
        configurationModified = true;
    }
    if (!theConfig.contains(Config::CFG_LAST_FILE_PATH)) {
        Config::setLastFilePath(jsonFilePath);
        configurationModified = true;
    }
    if (!theConfig.contains(Config::CFG_PAL_UNDEFINED_COLOR)) {
        Config::setPaletteUndefinedColor(Config::DEFAULT_PAL_UNDEFINED_COLOR);
        configurationModified = true;
    }
    if (!theConfig.contains(Config::CFG_PAL_SELECTION_BORDER_COLOR)) {
        Config::setPaletteSelectionBorderColor(Config::DEFAULT_PAL_SELECTION_BORDER_COLOR);
        configurationModified = true;
    }
    if (!theConfig.contains(Config::CFG_GRAPHICS_BACKGROUND_COLOR)) {
        Config::setGraphicsBackgroundColor(Config::DEFAULT_GRAPHICS_BACKGROUND_COLOR);
        configurationModified = true;
    }
    if (!theConfig.contains(Config::CFG_GRAPHICS_TRANSPARENT_COLOR)) {
        Config::setGraphicsTransparentColor(Config::DEFAULT_GRAPHICS_TRANSPARENT_COLOR);
        configurationModified = true;
    }
    if (configurationModified) {
        Config::storeConfiguration();
    }
}

bool Config::storeConfiguration()
{
    QString jsonFilePath = Config::getJsonFilePath();

    QFile saveJson(jsonFilePath);
    if (!saveJson.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QJsonDocument saveDoc(theConfig);
    saveJson.write(saveDoc.toJson());
    return true;
}

QJsonValue Config::value(const QString &name)
{
    return theConfig.value(name);
}

void Config::insert(const QString &key, const QJsonValue &value)
{
    theConfig.insert(key, value);
}
