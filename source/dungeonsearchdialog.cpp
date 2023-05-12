#include "dungeonsearchdialog.h"

#include <vector>

#include <QMessageBox>

#include "d1dun.h"
#include "levelcelview.h"
#include "progressdialog.h"
#include "ui_dungeonsearchdialog.h"

DungeonSearchDialog::DungeonSearchDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DungeonSearchDialog())
{
    ui->setupUi(this);
}

DungeonSearchDialog::~DungeonSearchDialog()
{
    delete ui;
}

void DungeonSearchDialog::initialize(D1Dun *dun)
{
    this->dun = dun;

    // this->ui->searchTypeComboBox->setCurrentIndex(0);
    // this->ui->searchIndexLineEdit->setText("");
    // this->ui->searchSpecCheckBox->setChecked(false);
    this->ui->replaceIndexLineEdit->setText("");
    this->ui->replaceSpecCheckBox->setChecked(false);

    if (this->ui->searchWLineEdit->text().isEmpty() || this->ui->searchHLineEdit->text().isEmpty()
        || this->ui->searchXLineEdit->nonNegInt() + this->ui->searchWLineEdit->nonNegInt() > dun->getWidth()
        || this->ui->searchYLineEdit->nonNegInt() + this->ui->searchHLineEdit->nonNegInt() > dun->getHeight()) {
        this->ui->searchXLineEdit->setText("0");
        this->ui->searchYLineEdit->setText("0");

        this->ui->searchWLineEdit->setText(QString::number(dun->getWidth()));
        this->ui->searchHLineEdit->setText(QString::number(dun->getHeight()));
    }
}

void DungeonSearchDialog::on_searchButton_clicked()
{
    DungeonSearchParam params;

    params.type = (DUN_SEARCH_TYPE)this->ui->searchTypeComboBox->currentIndex();
    params.index = this->ui->searchIndexLineEdit->text().toInt();
    params.replace = this->ui->replaceIndexLineEdit->text().toInt();
    params.doReplace = !this->ui->replaceIndexLineEdit->text().isEmpty();
    params.special = this->ui->searchSpecCheckBox->isChecked();
    params.replaceSpec = this->ui->replaceSpecCheckBox->isChecked();
    params.area.setX(this->ui->searchXLineEdit->nonNegInt());
    params.area.setY(this->ui->searchYLineEdit->nonNegInt());
    params.area.setWidth(this->ui->searchWLineEdit->nonNegInt());
    params.area.setHeight(this->ui->searchHLineEdit->nonNegInt());
    params.scrollTo = this->ui->scrollToCheckBox->isChecked();

    LevelCelView *view = qobject_cast<LevelCelView *>(this->parentWidget());

    QString typeTxt = this->ui->searchTypeComboBox->currentText();

    this->close();

    params.area = params.area.intersected(QRect(0, 0, this->dun->getWidth(), this->dun->getHeight()));

    if (params.area.width() == 0 || params.area.height() == 0) {
        return;
    }

    // do the search
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    QString msg = tr("Searching for %1 with index %2%3").arg(typeTxt).arg(params.index).arg(params.special ? "*" : "");
    if (params.area.x() != 0 || params.area.y() != 0 || params.area.width() != this->dun->getWidth() || params.area.height() != this->dun->getHeight()) {
        msg += tr(" starting from %1:%2").arg(params.area.x()).arg(params.area.y());
        if (params.area.x() + params.area.width() != this->dun->getWidth() || params.area.y() + params.area.height() != this->dun->getHeight()) {
            msg += tr(". The search is limited to %1 width and %2 height").arg(params.area.width()).arg(params.area.height());
        }
    }
    msg += ".";
    dProgress() << msg;

    std::vector<QPoint> matches;
    for (int posy = params.area.y(); posy < params.area.y() + params.area.height(); posy++) {
        for (int posx = params.area.x(); posx < params.area.x() + params.area.width(); posx++) {
            bool found = false;
            switch (params.type) {
            case DUN_SEARCH_TYPE::Tile:
                found = this->dun->getTileAt(posx, posy) == params.index;
                break;
            case DUN_SEARCH_TYPE::Subtile:
                found = this->dun->getSubtileAt(posx, posy) == params.index;
                break;
            case DUN_SEARCH_TYPE::Tile_Protection:
                found = this->dun->getTileProtectionAt(posx, posy) == (Qt::CheckState)params.index;
                break;
            case DUN_SEARCH_TYPE::Monster_Protection:
                found = this->dun->getSubtileMonProtectionAt(posx, posy);
                break;
            case DUN_SEARCH_TYPE::Object_Protection:
                found = this->dun->getSubtileObjProtectionAt(posx, posy);
                break;
            case DUN_SEARCH_TYPE::Room:
                found = this->dun->getRoomAt(posx, posy) == params.index;
                break;
            case DUN_SEARCH_TYPE::Monster: {
                DunMonsterType monType = this->dun->getMonsterAt(posx, posy);
                found = monType.first == params.index && monType.second == params.special;
            } break;
            case DUN_SEARCH_TYPE::Object:
                found = this->dun->getObjectAt(posx, posy) == params.index;
                break;
            case DUN_SEARCH_TYPE::Item:
                found = this->dun->getItemAt(posx, posy) == params.index;
                break;
            }
            if (found) {
                matches.push_back(QPoint(posx, posy));
            }
        }
    }
    if (matches.empty()) {
        dProgress() << tr("Not found.");
    } else {
        if (matches.size() > 4) {
            dProgress() << tr("Found at:");
            for (const QPoint &match : matches) {
                dProgress() << QString("    %1:%2").arg(match.x()).arg(match.y());
            }
        } else {
            QString msg = tr("Found at");
            for (const QPoint &match : matches) {
                msg += QString(" %1:%2,").arg(match.x()).arg(match.y());
            }
            msg[msg.length() - 1] = '.';
            dProgress() << msg;
        }
    }

    if (params.doReplace) {
        for (const QPoint &match : matches) {
            switch (params.type) {
            case DUN_SEARCH_TYPE::Tile:
                this->dun->setTileAt(match.x(), match.y(), params.replace);
                break;
            case DUN_SEARCH_TYPE::Subtile:
                this->dun->setSubtileAt(match.x(), match.y(), params.replace);
                break;
            case DUN_SEARCH_TYPE::Tile_Protection:
                this->dun->setTileProtectionAt(match.x(), match.y(), (Qt::CheckState)params.replace);
                break;
            case DUN_SEARCH_TYPE::Monster_Protection:
                this->dun->setSubtileMonProtectionAt(match.x(), match.y(), params.replace != 0);
                break;
            case DUN_SEARCH_TYPE::Object_Protection:
                this->dun->setSubtileObjProtectionAt(match.x(), match.y(), params.replace != 0);
                break;
            case DUN_SEARCH_TYPE::Room:
                this->dun->setRoomAt(match.x(), match.y(), params.replace);
                break;
            case DUN_SEARCH_TYPE::Monster: {
                DunMonsterType monType = { params.replace, params.replaceSpec };
                this->dun->setMonsterAt(match.x(), match.y(), monType);
            } break;
            case DUN_SEARCH_TYPE::Object:
                this->dun->setObjectAt(match.x(), match.y(), params.replace);
                break;
            case DUN_SEARCH_TYPE::Item:
                this->dun->setItemAt(match.x(), match.y(), params.replace);
                break;
            }
        }
        dProgress() << tr("Replaced with %1%2.").arg(params.replace).arg(params.replaceSpec ? "*" : "");
    }

    // Clear loading message from status bar
    ProgressDialog::done();

    if (!matches.empty() && params.scrollTo) {
        view->selectPos(matches[0]);
        view->scrollToCurrent();
    }
}

void DungeonSearchDialog::on_searchCancelButton_clicked()
{
    this->close();
}

void DungeonSearchDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}