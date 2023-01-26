#include "d1trn.h"

#include <QApplication>
#include <QFile>
#include <QMessageBox>

bool D1Trn::load(QString filePath, D1Pal *pal)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    if (file.size() != D1TRN_TRANSLATIONS)
        return false;

    int readBytes = file.read((char *)this->translations, D1TRN_TRANSLATIONS);
    if (readBytes != D1TRN_TRANSLATIONS)
        return false;

    this->palette = pal;
    this->refreshResultingPalette();

    this->trnFilePath = filePath;
    this->modified = false;
    return true;
}

bool D1Trn::save(QString filePath)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(filePath));
        return false;
    }

    file.write((char *)this->translations, D1TRN_TRANSLATIONS);

    if (this->trnFilePath == filePath) {
        this->modified = false;
    } else {
        // -- do not update, the user is creating a new one and the original needs to be preserved
        // this->modified = false;
        // this->trnFilePath = filePath;
    }
    return true;
}

bool D1Trn::isModified() const
{
    return this->modified;
}

void D1Trn::refreshResultingPalette()
{
    this->resultingPalette.setUndefinedColor(this->palette->getUndefinedColor());

    for (int i = 0; i < D1TRN_TRANSLATIONS; i++) {
        this->resultingPalette.setColor(
            i, this->palette->getColor(this->translations[i]));
    }
}

QColor D1Trn::getResultingColor(quint8 index)
{
    return this->resultingPalette.getColor(index);
}

QString D1Trn::getFilePath()
{
    return this->trnFilePath;
}

quint8 D1Trn::getTranslation(quint8 index)
{
    return this->translations[index];
}

void D1Trn::setTranslation(quint8 index, quint8 translation)
{
    this->translations[index] = translation;

    this->modified = true;
}

void D1Trn::setPalette(D1Pal *pal)
{
    this->palette = pal;
}

D1Pal *D1Trn::getResultingPalette()
{
    return &this->resultingPalette;
}
