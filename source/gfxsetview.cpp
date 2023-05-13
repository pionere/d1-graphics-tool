#include "gfxsetview.h"

#include <algorithm>

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "config.h"
#include "d1cl2.h"
#include "d1pcx.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_gfxsetview.h"
#include "upscaler.h"

GfxsetView::GfxsetView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GfxsetView())
{
    this->ui->setupUi(this);
    this->ui->celGraphicsView->setScene(&this->celScene);
    this->on_zoomEdit_escPressed();
    this->on_playDelayEdit_escPressed();
    QLayout *layout = this->ui->paintbuttonHorizontalLayout;
    PushButtonWidget *btn = PushButtonWidget::addButton(this, layout, QStyle::SP_DialogResetButton, tr("Start drawing"), &dMainWindow(), &MainWindow::on_actionToggle_Painter_triggered);
    layout->setAlignment(btn, Qt::AlignRight);

    this->loadGfxBtn = PushButtonWidget::addButton(this, QStyle::SP_DialogOpenButton, tr("Replace graphics"), this, &GfxsetView::on_loadGfxPushButtonClicked);

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->celScene, &CelScene::framePixelClicked, this, &GfxsetView::framePixelClicked);
    // QObject::connect(&this->celScene, &CelScene::framePixelHovered, this, &GfxsetView::framePixelHovered);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->groupIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_groupIndexEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

GfxsetView::~GfxsetView()
{
    delete ui;
    delete loadGfxBtn;
}

void GfxsetView::initialize(D1Pal *p, D1Gfxset *gs, bool bottomPanelHidden)
{
    this->pal = p;
    this->gfx = gs->getBaseGfx();
    this->gfxset = gs;

    this->ui->bottomPanel->setVisible(!bottomPanelHidden);

    this->update();
}

void GfxsetView::setPal(D1Pal *p)
{
    this->pal = p;
}

void GfxsetView::setGfx(D1Gfx *gfx)
{
    this->gfx = gfx;

    if (this->currentFrameIndex >= this->gfx->getFrameCount()) {
        this->currentFrameIndex = 0;
    }
    this->updateGroupIndex();
}

// Displaying CEL file path information
void GfxsetView::updateLabel()
{
    const int labelCount = this->gfxset->getGfxCount();
    QHBoxLayout *layout = this->ui->celLabelsHorizontalLayout; // TODO: use FlowLayout?
    while (layout->count() > labelCount + 1) {
        QLayoutItem *child = layout->takeAt(labelCount);
        delete child->widget();
        delete child;
    }
    while (layout->count() < labelCount + 1) {
        layout->insertWidget(0, new QLabel(""), 0, Qt::AlignLeft);
    }

    D1Gfx *baseGfx = this->gfxset->getBaseGfx();
    for (int i = 0; i < labelCount; i++) {
        D1Gfx *gfx = this->gfxset->getGfx(i);
        QLabel *label = qobject_cast<QLabel *>(layout->itemAt(i)->widget());
        CelView::setLabelContent(label, gfx->getFilePath(), gfx->isModified());
        if (gfx == baseGfx) {
            QString text = QString("<b>%1</b>").arg(label->text());
            label->setText(text);
        }
    }
}

void GfxsetView::update()
{
    int count;

    this->updateLabel();

    D1Gfxset *gs = this->gfxset;

    if (this->currType != gs->getType()) {
        this->currType = gs->getType();
        this->ui->misGfxsetPanel->setVisible(this->currType == D1GFX_SET_TYPE::Missile);
        this->ui->monGfxsetPanel->setVisible(this->currType == D1GFX_SET_TYPE::Monster);
        this->ui->plrGfxsetPanel->setVisible(this->currType == D1GFX_SET_TYPE::Player);
        if (this->currType == D1GFX_SET_TYPE::Missile) {
            qobject_cast<QGridLayout *>(this->ui->misGfxsetPanel->layout())->addWidget(this->loadGfxBtn, 2, 2, Qt::AlignCenter);
        } else if (this->currType == D1GFX_SET_TYPE::Monster) {
            qobject_cast<QGridLayout *>(this->ui->monGfxsetPanel->layout())->addWidget(this->loadGfxBtn, 2, 1, Qt::AlignCenter);
        } else {
            // assert(this->currType == D1GFX_SET_TYPE::Player);
            qobject_cast<QGridLayout *>(this->ui->plrGfxsetPanel->layout())->addWidget(this->loadGfxBtn, 4, 1, Qt::AlignCenter);
        }
        this->adjustSize();
    }

    QPushButton *buttons[16];
    unsigned numButtons = 0;
    if (this->currType == D1GFX_SET_TYPE::Missile) {
        if (gs->getGfxCount() == 16) {
            numButtons = 16;
            buttons[0] = this->ui->misSButton;
            buttons[1] = this->ui->misSSWButton;
            buttons[2] = this->ui->misSWButton;
            buttons[3] = this->ui->misWSWButton;
            buttons[4] = this->ui->misWButton;
            buttons[5] = this->ui->misWNWButton;
            buttons[6] = this->ui->misNWButton;
            buttons[7] = this->ui->misNNWButton;
            buttons[8] = this->ui->misNButton;
            buttons[9] = this->ui->misNNEButton;
            buttons[10] = this->ui->misNEButton;
            buttons[11] = this->ui->misENEButton;
            buttons[12] = this->ui->misEButton;
            buttons[13] = this->ui->misESEButton;
            buttons[14] = this->ui->misSEButton;
            buttons[15] = this->ui->misSSEButton;
        } else {
            // assert(gs->getGfxCount() == 8);
            numButtons = 8;
            buttons[DIR_S] = this->ui->misSButton;
            buttons[DIR_SW] = this->ui->misSWButton;
            buttons[DIR_W] = this->ui->misWButton;
            buttons[DIR_NW] = this->ui->misNWButton;
            buttons[DIR_N] = this->ui->misNButton;
            buttons[DIR_NE] = this->ui->misNEButton;
            buttons[DIR_E] = this->ui->misEButton;
            buttons[DIR_SE] = this->ui->misSEButton;
            this->ui->misNNWButton->setVisible(false);
            this->ui->misNNEButton->setVisible(false);
            this->ui->misWNWButton->setVisible(false);
            this->ui->misENEButton->setVisible(false);
            this->ui->misWSWButton->setVisible(false);
            this->ui->misESEButton->setVisible(false);
            this->ui->misSSWButton->setVisible(false);
            this->ui->misSSEButton->setVisible(false);
        }
    } else if (this->currType == D1GFX_SET_TYPE::Monster) {
        numButtons = 6;
        buttons[MA_STAND] = this->ui->monStandButton;
        buttons[MA_ATTACK] = this->ui->monAttackButton;
        buttons[MA_WALK] = this->ui->monWalkButton;
        buttons[MA_SPECIAL] = this->ui->monSpecButton;
        buttons[MA_GOTHIT] = this->ui->monHitButton;
        buttons[MA_DEATH] = this->ui->monDeathButton;
    } else {
        // assert(this->currType == D1GFX_SET_TYPE::Player);
        QString classLabel;
        switch (gs->getClassType()) {
        case D1GFX_SET_CLASS_TYPE::Warrior:
            classLabel = tr("Warrior");
            break;
        case D1GFX_SET_CLASS_TYPE::Rogue:
            classLabel = tr("Rogue");
            break;
        case D1GFX_SET_CLASS_TYPE::Mage:
            classLabel = tr("Mage");
            break;
        case D1GFX_SET_CLASS_TYPE::Monk:
            classLabel = tr("Monk");
            break;
        default:
            classLabel = tr("N/A");
        };
        this->ui->plrClassLabel->setText(classLabel);
        QString armorLabel;
        switch (gs->getArmorType()) {
        case D1GFX_SET_ARMOR_TYPE::Light:
            armorLabel = tr("Light");
            break;
        case D1GFX_SET_ARMOR_TYPE::Medium:
            armorLabel = tr("Medium");
            break;
        case D1GFX_SET_ARMOR_TYPE::Heavy:
            armorLabel = tr("Heavy");
            break;
        default:
            armorLabel = tr("N/A");
        };
        this->ui->plrArmorLabel->setText(armorLabel);
        QString weaponLabel;
        switch (gs->getWeaponType()) {
        case D1GFX_SET_WEAPON_TYPE::Unarmed:
            weaponLabel = tr("Unarmed");
            break;
        case D1GFX_SET_WEAPON_TYPE::ShieldOnly:
            weaponLabel = tr("Shield");
            break;
        case D1GFX_SET_WEAPON_TYPE::Sword:
            weaponLabel = tr("Sword");
            break;
        case D1GFX_SET_WEAPON_TYPE::SwordShield:
            weaponLabel = tr("Sword+");
            break;
        case D1GFX_SET_WEAPON_TYPE::Bow:
            weaponLabel = tr("Bow");
            break;
        case D1GFX_SET_WEAPON_TYPE::Axe:
            weaponLabel = tr("Axe");
            break;
        case D1GFX_SET_WEAPON_TYPE::Blunt:
            weaponLabel = tr("Blunt");
            break;
        case D1GFX_SET_WEAPON_TYPE::BluntShield:
            weaponLabel = tr("Blunt+");
            break;
        case D1GFX_SET_WEAPON_TYPE::Staff:
            weaponLabel = tr("Staff");
            break;
        default:
            weaponLabel = tr("N/A");
        };
        this->ui->plrWeaponLabel->setText(weaponLabel);

        numButtons = 11;
        buttons[PGT_STAND_TOWN] = this->ui->plrStandTownButton;
        buttons[PGT_STAND_DUNGEON] = this->ui->plrStandDunButton;
        buttons[PGT_WALK_TOWN] = this->ui->plrWalkTownButton;
        buttons[PGT_WALK_DUNGEON] = this->ui->plrWalkDunButton;
        buttons[PGT_ATTACK] = this->ui->plrAttackButton;
        buttons[PGT_BLOCK] = this->ui->plrBlockButton;
        buttons[PGT_FIRE] = this->ui->plrFireButton;
        buttons[PGT_MAGIC] = this->ui->plrMagicButton;
        buttons[PGT_LIGHTNING] = this->ui->plrLightButton;
        buttons[PGT_GOTHIT] = this->ui->plrHitButton;
        buttons[PGT_DEATH] = this->ui->plrDeathButton;
    }

    D1Gfx *baseGfx = gs->getBaseGfx();
    for (unsigned i = 0; i < numButtons; i++) {
        D1Gfx *gfx = gs->getGfx(i);
        buttons[i]->setToolTip(gfx->getFilePath());
        // buttons[i]->setDown(gfx == baseGfx);
        buttons[i]->setCheckable(gfx == baseGfx);
        buttons[i]->setChecked(gfx == baseGfx);
        buttons[i]->update();
    }

    // Set current and maximum group text
    count = this->gfx->getGroupCount();
    this->ui->groupIndexEdit->setText(
        QString::number(count != 0 ? this->currentGroupIndex + 1 : 0));
    this->ui->groupNumberEdit->setText(QString::number(count));

    // Set current and maximum frame text
    int frameIndex = this->currentFrameIndex;
    if (this->ui->framesGroupCheckBox->isChecked() && count != 0) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
        frameIndex -= groupFrameIndices.first;
        count = groupFrameIndices.second - groupFrameIndices.first + 1;
    } else {
        count = this->gfx->getFrameCount();
    }
    this->ui->frameIndexEdit->setText(
        QString::number(count != 0 ? frameIndex + 1 : 0));
    this->ui->frameNumberEdit->setText(QString::number(count));
}

CelScene *GfxsetView::getCelScene() const
{
    return const_cast<CelScene *>(&this->celScene);
}

int GfxsetView::getCurrentFrameIndex() const
{
    return this->currentFrameIndex;
}

void GfxsetView::framePixelClicked(const QPoint &pos, bool first)
{
    if (this->gfx->getFrameCount() == 0) {
        return;
    }
    D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
    QPoint p = pos;
    p -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    dMainWindow().frameClicked(frame, p, first);
}

void GfxsetView::framePixelHovered(const QPoint &pos)
{
}

void GfxsetView::insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevFrameCount = this->gfx->getFrameCount();

    if (append) {
        // append the frame(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertFrame(mode, this->gfx->getFrameCount(), imagefilePaths[i]);
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
        // jump to the first appended frame
        this->currentFrameIndex = prevFrameCount;
        this->updateGroupIndex();
    } else {
        // insert the frame(s)
        for (int i = imagefilePaths.count() - 1; i >= 0; i--) {
            this->insertFrame(mode, this->currentFrameIndex, imagefilePaths[i]);
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool wasModified = this->gfx->isModified();
        bool clipped, palMod;
        D1GfxFrame *frame = this->gfx->insertFrame(index, &clipped);
        if (!D1Pcx::load(*frame, imagefilePath, clipped, this->pal, this->gfx->getPalette(), &palMod)) {
            this->gfx->removeFrame(index);
            this->gfx->setModified(wasModified);
            QString msg = tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            if (mode != IMAGE_FILE_MODE::AUTO) {
                dProgressFail() << msg;
            } else {
                dProgressErr() << msg;
            }
        } else if (palMod) {
            // update the palette
            emit this->palModified();
        }
        return;
    }
    QImageReader reader = QImageReader(imagefilePath);
    if (!reader.canRead()) {
        QString msg = tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << msg;
        } else {
            dProgressErr() << msg;
        }
        return;
    }
    while (true) {
        QImage image = reader.read();
        if (image.isNull()) {
            break;
        }
        this->gfx->insertFrame(index, image);
        index++;
    }
}

void GfxsetView::addToCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
        if (!success) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        D1GfxFrame *resFrame = this->gfx->addToFrame(this->currentFrameIndex, frame);
        if (resFrame == nullptr) {
            return; // error set by gfx->addToFrame
        }
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    D1GfxFrame *frame = this->gfx->addToFrame(this->currentFrameIndex, image);
    if (frame == nullptr) {
        return; // error set by gfx->addToFrame
    }

    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::replaceCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = this->gfx->getFrame(this->currentFrameIndex)->isClipped(), palMod;
        D1GfxFrame *frame = new D1GfxFrame();
        bool success = D1Pcx::load(*frame, imagefilePath, clipped, this->pal, this->gfx->getPalette(), &palMod);
        if (!success) {
            delete frame;
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        this->gfx->setFrame(this->currentFrameIndex, frame);

        if (palMod) {
            // update the palette
            emit this->palModified();
        }
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    this->gfx->replaceFrame(this->currentFrameIndex, image);

    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::removeCurrentFrame()
{
    // remove the frame
    this->gfx->removeFrame(this->currentFrameIndex);
    if (this->gfx->getFrameCount() == this->currentFrameIndex) {
        this->currentFrameIndex = std::max(0, this->currentFrameIndex - 1);
    }
    this->updateGroupIndex();
    // update the view - done by the caller
    // this->displayFrame();
}

QImage GfxsetView::copyCurrent() const
{
    if (this->gfx->getFrameCount() == 0) {
        return QImage();
    }
    return this->gfx->getFrameImage(this->currentFrameIndex);
}

void GfxsetView::pasteCurrent(const QImage &image)
{
    if (this->gfx->getFrameCount() != 0) {
        this->gfx->replaceFrame(this->currentFrameIndex, image);
    } else {
        this->gfx->insertFrame(this->currentFrameIndex, image);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::resize(const ResizeParam &params)
{
    D1GfxPixel backPixel = (unsigned)params.backcolor < D1PAL_COLORS ? D1GfxPixel::colorPixel(params.backcolor) : D1GfxPixel::transparentPixel();
    int rangeFrom = params.rangeFrom;
    if (rangeFrom != 0) {
        rangeFrom--;
    }
    int rangeTo = params.rangeTo;
    if (rangeTo == 0 || rangeTo >= this->gfx->getFrameCount()) {
        rangeTo = this->gfx->getFrameCount();
    }
    rangeTo--;

    const RESIZE_PLACEMENT placement = params.placement;
    int frameWithPixelLost = -1;
    for (int i = rangeFrom; i <= rangeTo; i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        int width = params.width;
        int currWidth = frame->getWidth();
        if (width == 0) {
            width = currWidth;
        }
        int height = params.height;
        int currHeight = frame->getHeight();
        if (height == 0) {
            height = currHeight;
        }

        const std::vector<std::vector<D1GfxPixel>> &pixelLines = frame->getPixels();
        if (width < currWidth) {
            int counter = 0;
            for (int n = 0; n < currWidth - width; n++, counter++) {
                int idx;
                if (placement == RESIZE_PLACEMENT::TOP || placement == RESIZE_PLACEMENT::CENTER || placement == RESIZE_PLACEMENT::BOTTOM) {
                    if (counter & 1) {
                        idx = n / 2;
                    } else {
                        idx = currWidth - 1 - n / 2;
                    }
                } else if (placement == RESIZE_PLACEMENT::TOP_RIGHT || placement == RESIZE_PLACEMENT::CENTER_RIGHT || placement == RESIZE_PLACEMENT::BOTTOM_RIGHT) {
                    idx = n;
                } else {
                    idx = currWidth - 1 - n;
                }
                for (const std::vector<D1GfxPixel> &pixelLine : pixelLines) {
                    if (pixelLine[idx] != backPixel) {
                        frameWithPixelLost = i;
                        goto done;
                    }
                }
            }
        }

        if (height < currHeight) {
            int counter = 0;
            for (int n = 0; n < currHeight - height; n++, counter++) {
                int idx;
                if (placement == RESIZE_PLACEMENT::CENTER_LEFT || placement == RESIZE_PLACEMENT::CENTER || placement == RESIZE_PLACEMENT::CENTER_RIGHT) {
                    if (counter & 1) {
                        idx = n / 2;
                    } else {
                        idx = currHeight - 1 - n / 2;
                    }
                } else if (placement == RESIZE_PLACEMENT::BOTTOM_LEFT || placement == RESIZE_PLACEMENT::BOTTOM || placement == RESIZE_PLACEMENT::BOTTOM_RIGHT) {
                    idx = n;
                } else {
                    idx = currHeight - 1 - n;
                }
                const std::vector<D1GfxPixel> &pixelLine = pixelLines[idx];
                for (const D1GfxPixel &pixel : pixelLine) {
                    if (pixel != backPixel) {
                        frameWithPixelLost = i;
                        goto done;
                    }
                }
            }
        }
    }
done:
    if (frameWithPixelLost != -1) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Pixels with non-background colors are going to be eliminated (At least Frame %1 is affected). Are you sure you want to proceed?").arg(frameWithPixelLost + 1), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Resizing..."), 1, PAF_NONE);

    bool change = false;
    for (int i = rangeFrom; i <= rangeTo; i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        int width = params.width;
        int currWidth = frame->getWidth();
        if (width == 0)
            width = currWidth;
        int height = params.height;
        int currHeight = frame->getHeight();
        if (height == 0)
            height = currHeight;

        std::vector<std::vector<D1GfxPixel>> &pixelLines = frame->getPixels();
        int counter = 0;
        while (width > currWidth) {
            for (std::vector<D1GfxPixel> &pixelLine : pixelLines) {
                if ((placement == RESIZE_PLACEMENT::TOP_RIGHT || placement == RESIZE_PLACEMENT::CENTER_RIGHT || placement == RESIZE_PLACEMENT::BOTTOM_RIGHT)
                    || ((placement == RESIZE_PLACEMENT::CENTER || placement == RESIZE_PLACEMENT::TOP || placement == RESIZE_PLACEMENT::BOTTOM) && (counter & 1))) {
                    pixelLine.insert(pixelLine.begin(), backPixel);
                } else {
                    pixelLine.push_back(backPixel);
                }
            }
            counter++;
            currWidth++;
            change = true;
        }

        while (width < currWidth) {
            for (std::vector<D1GfxPixel> &pixelLine : pixelLines) {
                if ((placement == RESIZE_PLACEMENT::TOP_RIGHT || placement == RESIZE_PLACEMENT::CENTER_RIGHT || placement == RESIZE_PLACEMENT::BOTTOM_RIGHT)
                    || ((placement == RESIZE_PLACEMENT::CENTER || placement == RESIZE_PLACEMENT::TOP || placement == RESIZE_PLACEMENT::BOTTOM) && (counter & 1))) {
                    pixelLine.erase(pixelLine.begin());
                } else {
                    pixelLine.pop_back();
                }
            }
            counter++;
            currWidth--;
            change = true;
        }
        frame->setWidth(width);

        counter = 0;
        std::vector<D1GfxPixel> pixelLine;
        for (int x = 0; x < width; x++) {
            pixelLine.push_back(backPixel);
        }
        while (height > currHeight) {
            if ((placement == RESIZE_PLACEMENT::BOTTOM_LEFT || placement == RESIZE_PLACEMENT::BOTTOM || placement == RESIZE_PLACEMENT::BOTTOM_RIGHT)
                || ((placement == RESIZE_PLACEMENT::CENTER_LEFT || placement == RESIZE_PLACEMENT::CENTER || placement == RESIZE_PLACEMENT::CENTER_RIGHT) && (counter & 1))) {
                pixelLines.insert(pixelLines.begin(), pixelLine);
            } else {
                pixelLines.push_back(pixelLine);
            }
            counter++;
            currHeight++;
            change = true;
        }

        while (height < currHeight) {
            if ((placement == RESIZE_PLACEMENT::BOTTOM_LEFT || placement == RESIZE_PLACEMENT::BOTTOM || placement == RESIZE_PLACEMENT::BOTTOM_RIGHT)
                || ((placement == RESIZE_PLACEMENT::CENTER_LEFT || placement == RESIZE_PLACEMENT::CENTER || placement == RESIZE_PLACEMENT::CENTER_RIGHT) && (counter & 1))) {
                pixelLines.erase(pixelLines.begin());
            } else {
                pixelLines.pop_back();
            }
            counter++;
            currHeight--;
            change = true;
        }
        frame->setHeight(height);
    }

    ProgressDialog::done();

    if (change) {
        this->gfx->setModified();
        // update the view
        this->displayFrame();
    }
}

void GfxsetView::upscale(const UpscaleParam &params)
{
    if (Upscaler::upscaleGfx(this->gfx, params, false)) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void GfxsetView::displayFrame()
{
    this->update();

    this->celScene.clear();

    // Getting the current frame to display
    QImage celFrame = this->gfx->getFrameCount() != 0 ? this->gfx->getFrameImage(this->currentFrameIndex) : QImage();

    this->celScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32);
    celFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->celScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + celFrame.width() + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + celFrame.height() + CEL_SCENE_MARGIN);
    // ui->celGraphicsView->adjustSize();

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene.addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    this->celScene.addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);

    // Set current frame width and height
    this->ui->celFrameWidthEdit->setText(QString::number(celFrame.width()) + " px");
    this->ui->celFrameHeightEdit->setText(QString::number(celFrame.height()) + " px");

    // Notify PalView that the frame changed (used to refresh palette widget)
    emit this->frameRefreshed();
}

void GfxsetView::toggleBottomPanel()
{
    this->ui->bottomPanel->setVisible(this->ui->bottomPanel->isHidden());
}

void GfxsetView::changeColor(const std::vector<std::pair<D1GfxPixel, D1GfxPixel>> &replacements, bool all)
{
    if (all) {
        for (int n = 0; n < this->gfxset->getGfxCount(); n++) {
            D1Gfx *gfx = this->gfxset->getGfx(n);
            for (int i = 0; i < gfx->getFrameCount(); i++) {
                D1GfxFrame *frame = gfx->getFrame(i);
                frame->replacePixels(replacements);
            }
            gfx->setModified();
        }
    } else {
        if (this->gfx->getFrameCount() == 0) {
            return;
        }
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        frame->replacePixels(replacements);
        this->gfx->setModified();
    }
}

void GfxsetView::updateGroupIndex()
{
    int i = 0;

    for (; i < this->gfx->getGroupCount(); i++) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(i);

        if (this->currentFrameIndex >= groupFrameIndices.first
            && this->currentFrameIndex <= groupFrameIndices.second) {
            break;
        }
    }
    this->currentGroupIndex = i;
}

void GfxsetView::setFrameIndex(int frameIndex)
{
    const int frameCount = this->gfx->getFrameCount();
    if (frameCount == 0) {
        // this->currentFrameIndex = 0;
        // this->currentGroupIndex = 0;
        // this->displayFrame();
        return;
    }
    if (frameIndex >= frameCount) {
        frameIndex = frameCount - 1;
    } else if (frameIndex < 0) {
        frameIndex = 0;
    }
    this->currentFrameIndex = frameIndex;
    this->updateGroupIndex();

    this->displayFrame();
}

void GfxsetView::setGroupIndex(int groupIndex)
{
    const int groupCount = this->gfx->getGroupCount();
    if (groupCount == 0) {
        // this->currentFrameIndex = 0;
        // this->currentGroupIndex = 0;
        // this->displayFrame();
        return;
    }
    if (groupIndex >= groupCount) {
        groupIndex = groupCount - 1;
    } else if (groupIndex < 0) {
        groupIndex = 0;
    }
    std::pair<int, int> prevGroupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
    int frameIndex = this->currentFrameIndex - prevGroupFrameIndices.first;
    std::pair<int, int> newGroupFrameIndices = this->gfx->getGroupFrameIndices(groupIndex);
    this->currentGroupIndex = groupIndex;
    this->currentFrameIndex = std::min(newGroupFrameIndices.first + frameIndex, newGroupFrameIndices.second);

    this->displayFrame();
}

void GfxsetView::ShowContextMenu(const QPoint &pos)
{
    MainWindow *mw = &dMainWindow();
    QAction actions[7];

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    // 'Frame' submenu of 'Edit'
    int cursor = 0;
    actions[cursor].setText(tr("Add Layer"));
    actions[cursor].setToolTip(tr("Add the content of an image to the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddTo_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert Frame"));
    actions[cursor].setToolTip(tr("Add new frames before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Append Frame"));
    actions[cursor].setToolTip(tr("Append new frames at the end"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAdd_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Replace Frame"));
    actions[cursor].setToolTip(tr("Replace the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionReplace_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Del Frame"));
    actions[cursor].setToolTip(tr("Delete the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDel_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    contextMenu.exec(mapToGlobal(pos));
}

void GfxsetView::selectGfx(int gfxIndex)
{
    dMainWindow().gfxChanged(this->gfxset->getGfx(gfxIndex));
}

void GfxsetView::on_misNWButton_clicked()
{
    this->selectGfx(6);
}

void GfxsetView::on_misNNWButton_clicked()
{
    this->selectGfx(7);
}

void GfxsetView::on_misNButton_clicked()
{
    this->selectGfx(8);
}

void GfxsetView::on_misNNEButton_clicked()
{
    this->selectGfx(9);
}

void GfxsetView::on_misNEButton_clicked()
{
    this->selectGfx(10);
}

void GfxsetView::on_misWNWButton_clicked()
{
    this->selectGfx(5);
}

void GfxsetView::on_misENEButton_clicked()
{
    this->selectGfx(11);
}

void GfxsetView::on_misWButton_clicked()
{
    this->selectGfx(4);
}

void GfxsetView::on_misEButton_clicked()
{
    this->selectGfx(12);
}

void GfxsetView::on_misWSWButton_clicked()
{
    this->selectGfx(3);
}

void GfxsetView::on_misESEButton_clicked()
{
    this->selectGfx(13);
}

void GfxsetView::on_misSWButton_clicked()
{
    this->selectGfx(2);
}

void GfxsetView::on_misSSWButton_clicked()
{
    this->selectGfx(1);
}

void GfxsetView::on_misSButton_clicked()
{
    this->selectGfx(0);
}

void GfxsetView::on_misSSEButton_clicked()
{
    this->selectGfx(15);
}

void GfxsetView::on_misSEButton_clicked()
{
    this->selectGfx(14);
}

void GfxsetView::on_monStandButton_clicked()
{
    this->selectGfx(MA_STAND);
}

void GfxsetView::on_monAttackButton_clicked()
{
    this->selectGfx(MA_ATTACK);
}

void GfxsetView::on_monWalkButton_clicked()
{
    this->selectGfx(MA_WALK);
}

void GfxsetView::on_monSpecButton_clicked()
{
    this->selectGfx(MA_SPECIAL);
}

void GfxsetView::on_monHitButton_clicked()
{
    this->selectGfx(MA_GOTHIT);
}

void GfxsetView::on_monDeathButton_clicked()
{
    this->selectGfx(MA_DEATH);
}

void GfxsetView::on_plrStandTownButton_clicked()
{
    this->selectGfx(PGT_STAND_TOWN);
}

void GfxsetView::on_plrStandDunButton_clicked()
{
    this->selectGfx(PGT_STAND_DUNGEON);
}

void GfxsetView::on_plrWalkTownButton_clicked()
{
    this->selectGfx(PGT_WALK_TOWN);
}

void GfxsetView::on_plrWalkDunButton_clicked()
{
    this->selectGfx(PGT_WALK_DUNGEON);
}

void GfxsetView::on_plrAttackButton_clicked()
{
    this->selectGfx(PGT_ATTACK);
}

void GfxsetView::on_plrBlockButton_clicked()
{
    this->selectGfx(PGT_BLOCK);
}

void GfxsetView::on_plrFireButton_clicked()
{
    this->selectGfx(PGT_FIRE);
}

void GfxsetView::on_plrMagicButton_clicked()
{
    this->selectGfx(PGT_MAGIC);
}

void GfxsetView::on_plrLightButton_clicked()
{
    this->selectGfx(PGT_LIGHTNING);
}

void GfxsetView::on_plrHitButton_clicked()
{
    this->selectGfx(PGT_GOTHIT);
}

void GfxsetView::on_plrDeathButton_clicked()
{
    this->selectGfx(PGT_DEATH);
}

void GfxsetView::on_loadGfxPushButtonClicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Load Graphics"), tr("CL2 Files (*.cl2 *.CL2)"));

    if (!openFilePath.isEmpty()) {
        D1Gfx *baseGfx = this->gfxset->getBaseGfx();
        QString currFilePath = baseGfx->getFilePath();

        OpenAsParam params = OpenAsParam();
        params.gfxType = OPEN_GFX_TYPE::BASIC;
        params.celFilePath = openFilePath;
        D1Cl2::load(*baseGfx, openFilePath, params);
        if (baseGfx->getFilePath() != currFilePath) {
            baseGfx->setFilePath(currFilePath);
        }
        dMainWindow().updateWindow();
    }
}

void GfxsetView::on_framesGroupCheckBox_clicked()
{
    // update frameIndexEdit and frameNumberEdit
    this->update();
}

void GfxsetView::on_firstFrameButton_clicked()
{
    int nextFrameIndex = 0;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxsetView::on_previousFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex - 1;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::max(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).first);
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxsetView::on_nextFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex + 1;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::min(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).second);
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxsetView::on_lastFrameButton_clicked()
{
    int nextFrameIndex = this->gfx->getFrameCount() - 1;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).second;
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxsetView::on_frameIndexEdit_returnPressed()
{
    int nextFrameIndex = this->ui->frameIndexEdit->text().toInt() - 1;

    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
        nextFrameIndex += groupFrameIndices.first;
        nextFrameIndex = std::max(nextFrameIndex, groupFrameIndices.first);
        nextFrameIndex = std::min(nextFrameIndex, groupFrameIndices.second);
    }
    this->setFrameIndex(nextFrameIndex);

    this->on_frameIndexEdit_escPressed();
}

void GfxsetView::on_frameIndexEdit_escPressed()
{
    // update frameIndexEdit
    this->update();
    this->ui->frameIndexEdit->clearFocus();
}

void GfxsetView::on_firstGroupButton_clicked()
{
    this->setGroupIndex(0);
}

void GfxsetView::on_previousGroupButton_clicked()
{
    this->setGroupIndex(this->currentGroupIndex - 1);
}

void GfxsetView::on_groupIndexEdit_returnPressed()
{
    int groupIndex = this->ui->groupIndexEdit->text().toInt() - 1;

    this->setGroupIndex(groupIndex);

    this->on_groupIndexEdit_escPressed();
}

void GfxsetView::on_groupIndexEdit_escPressed()
{
    // update groupIndexEdit
    this->update();
    this->ui->groupIndexEdit->clearFocus();
}

void GfxsetView::on_nextGroupButton_clicked()
{
    this->setGroupIndex(this->currentGroupIndex + 1);
}

void GfxsetView::on_lastGroupButton_clicked()
{
    this->setGroupIndex(this->gfx->getGroupCount() - 1);
}

void GfxsetView::on_zoomOutButton_clicked()
{
    this->celScene.zoomOut();
    this->on_zoomEdit_escPressed();
}

void GfxsetView::on_zoomInButton_clicked()
{
    this->celScene.zoomIn();
    this->on_zoomEdit_escPressed();
}

void GfxsetView::on_zoomEdit_returnPressed()
{
    QString zoom = this->ui->zoomEdit->text();

    this->celScene.setZoom(zoom);

    this->on_zoomEdit_escPressed();
}

void GfxsetView::on_zoomEdit_escPressed()
{
    this->ui->zoomEdit->setText(this->celScene.zoomText());
    this->ui->zoomEdit->clearFocus();
}

void GfxsetView::on_playDelayEdit_returnPressed()
{
    quint16 playDelay = this->ui->playDelayEdit->text().toUInt();

    if (playDelay != 0)
        this->currentPlayDelay = playDelay;

    this->on_playDelayEdit_escPressed();
}

void GfxsetView::on_playDelayEdit_escPressed()
{
    this->ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));
    this->ui->playDelayEdit->clearFocus();
}

void GfxsetView::on_playStopButton_clicked()
{
    if (this->playTimer != 0) {
        this->killTimer(this->playTimer);
        this->playTimer = 0;

        // restore the currentFrameIndex
        this->currentFrameIndex = this->origFrameIndex;
        // restore palette
        dMainWindow().resetPaletteCycle();
        // change the label of the button
        this->ui->playStopButton->setText(tr("Play"));
        // enable the related fields
        this->ui->playDelayEdit->setReadOnly(false);
        this->ui->playComboBox->setEnabled(true);
        return;
    }
    // disable the related fields
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(false);
    // change the label of the button
    this->ui->playStopButton->setText(tr("Stop"));
    // preserve the currentFrameIndex
    this->origFrameIndex = this->currentFrameIndex;
    // preserve the palette
    dMainWindow().initPaletteCycle();

    this->playTimer = this->startTimer(this->currentPlayDelay);
}

void GfxsetView::timerEvent(QTimerEvent *event)
{
    if (this->gfx->getGroupCount() == 0) {
        return;
    }
    std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);

    int nextFrameIndex = this->currentFrameIndex + 1;
    Qt::CheckState playType = this->ui->playFrameCheckBox->checkState();
    if (playType == Qt::Unchecked) {
        // normal playback
        if (nextFrameIndex > groupFrameIndices.second)
            nextFrameIndex = groupFrameIndices.first;
    } else if (playType == Qt::PartiallyChecked) {
        // playback till the original frame
        if (nextFrameIndex > this->origFrameIndex)
            nextFrameIndex = groupFrameIndices.first;
    } else {
        // playback from the original frame
        if (nextFrameIndex > groupFrameIndices.second)
            nextFrameIndex = this->origFrameIndex;
    }
    this->currentFrameIndex = nextFrameIndex;
    int cycleType = this->ui->playComboBox->currentIndex();
    if (cycleType == 0) {
        // normal playback
        this->displayFrame();
    } else {
        dMainWindow().nextPaletteCycle((D1PAL_CYCLE_TYPE)(cycleType - 1));
        // this->displayFrame();
    }
}

void GfxsetView::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void GfxsetView::dragMoveEvent(QDragMoveEvent *event)
{
    if (MainWindow::hasImageUrl(event->mimeData())) {
        event->acceptProposedAction();
    }
}

void GfxsetView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    dMainWindow().openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}
