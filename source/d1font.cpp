#include "d1font.h"

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QMessageBox>
#include <QPainter>

#include "progressdialog.h"

bool D1Font::load(D1Gfx &gfx, const QString &filePath, const ImportParam &params)
{
    gfx.clear();

    int fontId = QFontDatabase::addApplicationFont(filePath);
    if (fontId == -1) {
        dProgressErr() <<  QApplication::tr("Font could not be loaded.");
        return false;
    }

    QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.size() == 0) {
        dProgressErr() <<  QApplication::tr("No font families loaded.");
        QFontDatabase::removeApplicationFont(fontId);
        return false;
    }

    int pointSize = params.fontSize;
    QFont font = QFont(families[0], pointSize);
    QFontMetrics metrics = QFontMetrics(font);

    QColor renderColor = gfx.getPalette()->getColor(params.fontColor);
    int cursor = 0;
    for (int i = params.fontRangeFrom; i < params.fontRangeTo; i++, cursor++) {
        char32_t codePoint = static_cast<char32_t>(i);
        QString text = QString::fromUcs4(&codePoint, 1);
        QSize renderSize = metrics.size(0, text);
        QImage image = QImage(renderSize, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter = QPainter(&image);
        painter.setPen(renderColor);
        painter.setFont(font);
        painter.drawText(0, metrics.ascent(), text);

        gfx.insertFrame(cursor, image);
    }
    QFontDatabase::removeApplicationFont(fontId);

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    return true;
}
