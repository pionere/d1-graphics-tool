#include "config.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

static QJsonObject theConfig;

QString Config::getJsonFilePath()
{
    return QCoreApplication::applicationDirPath() + Config::FILE_PATH;
}

void Config::loadConfiguration()
{
    QString jsonFilePath = Config::getJsonFilePath();
    bool configurationModified = false;

    // If configuration file exists load it otherwise create it
    if (QFile::exists(jsonFilePath)) {
        QFile loadJson(jsonFilePath);
        loadJson.open(QIODevice::ReadOnly);
        QJsonDocument loadJsonDoc = QJsonDocument::fromJson(loadJson.readAll());
        theConfig = loadJsonDoc.object();
        loadJson.close();
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

    if (configurationModified) {
        Config::storeConfiguration();
    }
}

void Config::storeConfiguration()
{
    QString jsonFilePath = Config::getJsonFilePath();

    QFile saveJson(jsonFilePath);
    saveJson.open(QIODevice::WriteOnly);
    QJsonDocument saveDoc(theConfig);
    saveJson.write(saveDoc.toJson());
    saveJson.close();
}

QJsonValue Config::value(const QString &name)
{
    return theConfig.value(name);
}

void Config::insert(const QString &key, const QJsonValue &value)
{
    theConfig.insert(key, value);
}
