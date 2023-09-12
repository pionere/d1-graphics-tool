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
    this->index = -1;

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
    this->search(false);
}

void DungeonSearchDialog::on_searchNextButton_clicked()
{
    this->search(true);
}

void DungeonSearchDialog::on_searchCancelButton_clicked()
{
    this->close();
}

void DungeonSearchDialog::search(bool next)
{
    DungeonSearchParam params;

    params.type = (DUN_SEARCH_TYPE)this->ui->searchTypeComboBox->currentIndex();
    bool ok;
    params.index = this->ui->searchIndexLineEdit->text().toInt(&ok);
    if (!ok) {
        if (params.type == DUN_SEARCH_TYPE::Tile) {
            params.index = UNDEF_TILE;
        } else if (params.type == DUN_SEARCH_TYPE::Subtile) {
            params.index = UNDEF_SUBTILE;
        }
    }
    params.replace = this->ui->replaceIndexLineEdit->text().toInt();
    params.doReplace = !this->ui->replaceIndexLineEdit->text().isEmpty();
    params.special = this->ui->searchSpecCheckBox->isChecked();
    params.replaceSpec = this->ui->replaceSpecCheckBox->isChecked();
    params.area.setX(this->ui->searchXLineEdit->nonNegInt());
    params.area.setY(this->ui->searchYLineEdit->nonNegInt());
    params.area.setWidth(this->ui->searchWLineEdit->nonNegInt());
    params.area.setHeight(this->ui->searchHLineEdit->nonNegInt());
    params.scrollTo = this->ui->scrollToCheckBox->isChecked() || next;

    LevelCelView *view = qobject_cast<LevelCelView *>(this->parentWidget());

    QString typeTxt = this->ui->searchTypeComboBox->currentText();

    this->index++;
    if (!next) {
        this->close();
    }

    params.area = params.area.intersected(QRect(0, 0, this->dun->getWidth(), this->dun->getHeight()));

    // do the search
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, next ? PAF_NONE : PAF_OPEN_DIALOG);

    QString msg = tr("Searching for %1 with index %2%3").arg(typeTxt).arg(params.index).arg(params.special ? "*" : "");
    if (params.type == DUN_SEARCH_TYPE::Tile && params.index == UNDEF_TILE) {
        msg = tr("Searching for Undefined Tiles");
    } else if (params.type == DUN_SEARCH_TYPE::Subtile && params.index == UNDEF_SUBTILE) {
        msg = tr("Searching for Undefined Subtiles");
    }
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
                found = ((unsigned)posx % TILE_WIDTH) == 0 && ((unsigned)posy % TILE_HEIGHT) == 0 && this->dun->getTileAt(posx, posy) == params.index;
                break;
            case DUN_SEARCH_TYPE::Subtile:
                found = this->dun->getSubtileAt(posx, posy) == params.index;
                break;
            case DUN_SEARCH_TYPE::Tile_Protection:
                found = ((unsigned)posx % TILE_WIDTH) == 0 && ((unsigned)posy % TILE_HEIGHT) == 0 && this->dun->getTileProtectionAt(posx, posy) == (Qt::CheckState)params.index;
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
                DunMonsterType monType = this->dun->getMonsterAt(posx, posy).type;
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

    int matchIdx = this->index;
    bool overflow = matches.size() <= (unsigned)matchIdx;
    if (overflow) {
        matchIdx = matches.size() != 0 ? 0 : -1;
    }
    bool replaced = false;
    if (params.doReplace) {
        if (next) {
            if (!overflow && this->replace(matches[matchIdx], params)) {
                dProgress() << tr("Replaced with %1%2 at %3:%4.").arg(params.replace).arg(params.replaceSpec ? "*" : "").arg(matches[matchIdx].x()).arg(matches[matchIdx].y());
                replaced = true;
                this->index = matchIdx - 1;
            }
        } else {
            for (const QPoint &match : matches) {
                 replaced |= this->replace(match, params);
            }
            if (replaced) {
                dProgress() << tr("Replaced with %1%2.").arg(params.replace).arg(params.replaceSpec ? "*" : "");
            }
        }
    }

    bool popup = false;
    if (next) {
        if (overflow) {
            this->index = -1;
            params.scrollTo = false;
            if (matchIdx == -1) {
                popup = true;
            } else {
                dProgress() << tr("Reached the end of the dungeon.");
            }
        }
    }

    // Clear loading message from status bar
    ProgressDialog::done();

    // update the view
    if (replaced) {
        view->displayFrame();
    }
    if (matchIdx != -1 && params.scrollTo) {
        view->selectPos(matches[matchIdx], DOUBLE_CLICK);
    }
    if (popup) {
        QMessageBox::warning(this, "Warning", "Not found.");
    }
}

bool DungeonSearchDialog::replace(const QPoint &match, const DungeonSearchParam &params)
{
    D1Dun *dun = this->dun;
    int replace = params.replace;

    bool result = false;
    switch (params.type) {
    case DUN_SEARCH_TYPE::Tile:
        result = dun->setTileAt(match.x(), match.y(), replace);
        break;
    case DUN_SEARCH_TYPE::Subtile:
        result = dun->setSubtileAt(match.x(), match.y(), replace);
        break;
    case DUN_SEARCH_TYPE::Tile_Protection:
        result = dun->setTileProtectionAt(match.x(), match.y(), (Qt::CheckState)replace);
        break;
    case DUN_SEARCH_TYPE::Monster_Protection:
        result = dun->setSubtileMonProtectionAt(match.x(), match.y(), replace != 0);
        break;
    case DUN_SEARCH_TYPE::Object_Protection:
        result = dun->setSubtileObjProtectionAt(match.x(), match.y(), replace != 0);
        break;
    case DUN_SEARCH_TYPE::Room:
        result = dun->setRoomAt(match.x(), match.y(), replace);
        break;
    case DUN_SEARCH_TYPE::Monster: {
        DunMonsterType monType = { replace, params.replaceSpec };
        MapMonster mon = dun->getMonsterAt(match.x(), match.y());
        result = dun->setMonsterAt(match.x(), match.y(), monType, mon.mox, mon.moy);
    } break;
    case DUN_SEARCH_TYPE::Object:
        result = dun->setObjectAt(match.x(), match.y(), replace);
        break;
    case DUN_SEARCH_TYPE::Item:
        result = dun->setItemAt(match.x(), match.y(), replace);
        break;
    }
    return result;
}

void DungeonSearchDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
