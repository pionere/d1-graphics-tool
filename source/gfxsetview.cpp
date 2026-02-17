#include "gfxsetview.h"

#include <algorithm>

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QScrollBar>

#include "config.h"
#include "d1cel.h"
#include "d1cl2.h"
#include "d1pcx.h"
#include "gfxcomponentdialog.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_gfxsetview.h"
#include "upscaler.h"
#include "dungeon/all.h"

GfxsetView::GfxsetView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GfxsetView())
{
    this->ui->setupUi(this);
    this->ui->celGraphicsView->setScene(&this->celScene);
    this->ui->celGraphicsView->setMouseTracking(true);
    QScrollBar *sb;
    sb = this->ui->celGraphicsView->horizontalScrollBar();
    sb->installEventFilter(new WheelEventFilter(sb));
    sb = this->ui->celGraphicsView->verticalScrollBar();
    sb->installEventFilter(new WheelEventFilter(sb));

    this->on_zoomEdit_escPressed();
    // this->on_playDelayEdit_escPressed();
    // this->on_assetMplEdit_escPressed();
    QLayout *compLayout = this->ui->componentsHorizontalLayout;
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_FileDialogNewFolder, tr("New Component"), this, &GfxsetView::on_newComponentPushButtonClicked);
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_DialogOpenButton, tr("Edit Component"), this, &GfxsetView::on_editComponentPushButtonClicked);
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_BrowserReload, tr("Reload Component Graphics"), this, &GfxsetView::on_reloadComponentPushButtonClicked);
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_DialogCloseButton, tr("Remove Component"), this, &GfxsetView::on_closeComponentPushButtonClicked);

    QLayout *layout = this->ui->paintbuttonHorizontalLayout;
    PushButtonWidget *btn = PushButtonWidget::addButton(this, layout, QStyle::SP_DialogResetButton, tr("Start drawing"), &dMainWindow(), &MainWindow::on_actionToggle_Painter_triggered);
    layout->setAlignment(btn, Qt::AlignRight);

    layout = this->ui->switchToMetaHorizontalLayout;
    btn = PushButtonWidget::addButton(this, layout, QStyle::SP_ArrowRight, tr("Switch to metadata view"), this, &GfxsetView::on_actionToggle_Mode_triggered);
    layout->setAlignment(btn, Qt::AlignRight);

    layout = this->ui->switchToDisplayHorizontalLayout;
    btn = PushButtonWidget::addButton(this, layout, QStyle::SP_ArrowLeft, tr("Switch to display view"), this, &GfxsetView::on_actionToggle_Mode_triggered);
    layout->setAlignment(btn, Qt::AlignRight);

    QTextEdit *edit = this->ui->animOrderEdit;
    QFontMetrics fm = this->fontMetrics();
    int RowHeight = fm.lineSpacing() ;
    const QMargins qm = edit->contentsMargins();
    QGridLayout *grid = this->ui->animOrderGridLayout;
    edit->setFixedHeight(2 * RowHeight + qm.top() + qm.bottom() + grid->verticalSpacing());

    this->loadGfxBtn = PushButtonWidget::addButton(this, QStyle::SP_FileDialogNewFolder, tr("Replace graphics"), this, &GfxsetView::on_loadGfxPushButtonClicked);

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->celScene, &CelScene::framePixelClicked, this, &GfxsetView::framePixelClicked);
    // QObject::connect(&this->celScene, &CelScene::framePixelHovered, this, &GfxsetView::framePixelHovered);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->groupIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_groupIndexEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));
    QObject::connect(this->ui->assetMplEdit, SIGNAL(cancel_signal()), this, SLOT(on_assetMplEdit_escPressed()));
    QObject::connect(this->ui->metaFrameWidthEdit, SIGNAL(cancel_signal()), this, SLOT(on_metaFrameWidthEdit_escPressed()));
    QObject::connect(this->ui->metaFrameHeightEdit, SIGNAL(cancel_signal()), this, SLOT(on_metaFrameHeightEdit_escPressed()));
    QObject::connect(this->ui->animDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_animDelayEdit_escPressed()));
    QObject::connect(this->ui->actionFramesEdit, SIGNAL(cancel_signal()), this, SLOT(on_actionFramesEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

GfxsetView::~GfxsetView()
{
    delete gfxComponentDialog;
    delete ui;
    delete loadGfxBtn;
}

void GfxsetView::initialize(D1Pal *p, D1Gfxset *gs, bool bottomPanelHidden)
{
    this->pal = p;
    this->gfx = gs->getBaseGfx();
    this->gfxset = gs;

    this->ui->bottomPanel->setVisible(!bottomPanelHidden);

    {
        const D1GfxMeta *meta = this->gfx->getMeta(CELMETA_ANIMDELAY);
        if (meta->isStored()) {
            this->currentPlayDelay = meta->getContent().toInt() * (1000000 / SPEED_NORMAL);
        }
    }

    this->updateFields();
}

void GfxsetView::setPal(D1Pal *p)
{
    this->pal = p;
}

void GfxsetView::setGfx(D1Gfx *gfx)
{
    // stop playback
    if (this->playTimer != 0) {
        this->on_playStopButton_clicked();
    }

    this->gfx = gfx;

    {
        const D1GfxMeta *meta = gfx->getMeta(CELMETA_ANIMDELAY);
        if (meta->isStored()) {
            this->currentPlayDelay = meta->getContent().toInt() * (1000000 / SPEED_NORMAL);
        }
    }

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

void GfxsetView::updateFields()
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

    unsigned numButtons = 0;
    if (this->currType == D1GFX_SET_TYPE::Missile) {
        if (gs->getGfxCount() == 16) {
            numButtons = 16;
            this->buttons[0] = this->ui->misSButton;
            this->buttons[1] = this->ui->misSSWButton;
            this->buttons[2] = this->ui->misSWButton;
            this->buttons[3] = this->ui->misWSWButton;
            this->buttons[4] = this->ui->misWButton;
            this->buttons[5] = this->ui->misWNWButton;
            this->buttons[6] = this->ui->misNWButton;
            this->buttons[7] = this->ui->misNNWButton;
            this->buttons[8] = this->ui->misNButton;
            this->buttons[9] = this->ui->misNNEButton;
            this->buttons[10] = this->ui->misNEButton;
            this->buttons[11] = this->ui->misENEButton;
            this->buttons[12] = this->ui->misEButton;
            this->buttons[13] = this->ui->misESEButton;
            this->buttons[14] = this->ui->misSEButton;
            this->buttons[15] = this->ui->misSSEButton;
        } else {
            // assert(gs->getGfxCount() == 8);
            numButtons = NUM_DIRS;
            this->buttons[DIR_S] = this->ui->misSButton;
            this->buttons[DIR_SW] = this->ui->misSWButton;
            this->buttons[DIR_W] = this->ui->misWButton;
            this->buttons[DIR_NW] = this->ui->misNWButton;
            this->buttons[DIR_N] = this->ui->misNButton;
            this->buttons[DIR_NE] = this->ui->misNEButton;
            this->buttons[DIR_E] = this->ui->misEButton;
            this->buttons[DIR_SE] = this->ui->misSEButton;
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
        numButtons = NUM_MON_ANIM;
        this->buttons[MA_STAND] = this->ui->monStandButton;
        this->buttons[MA_ATTACK] = this->ui->monAttackButton;
        this->buttons[MA_WALK] = this->ui->monWalkButton;
        this->buttons[MA_SPECIAL] = this->ui->monSpecButton;
        this->buttons[MA_GOTHIT] = this->ui->monHitButton;
        this->buttons[MA_DEATH] = this->ui->monDeathButton;
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

        numButtons = NUM_PGTS;
        this->buttons[PGT_STAND_TOWN] = this->ui->plrStandTownButton;
        this->buttons[PGT_STAND_DUNGEON] = this->ui->plrStandDunButton;
        this->buttons[PGT_WALK_TOWN] = this->ui->plrWalkTownButton;
        this->buttons[PGT_WALK_DUNGEON] = this->ui->plrWalkDunButton;
        this->buttons[PGT_ATTACK] = this->ui->plrAttackButton;
        this->buttons[PGT_BLOCK] = this->ui->plrBlockButton;
        this->buttons[PGT_FIRE] = this->ui->plrFireButton;
        this->buttons[PGT_MAGIC] = this->ui->plrMagicButton;
        this->buttons[PGT_LIGHTNING] = this->ui->plrLightButton;
        this->buttons[PGT_GOTHIT] = this->ui->plrHitButton;
        this->buttons[PGT_DEATH] = this->ui->plrDeathButton;
    }

    D1Gfx *baseGfx = gs->getBaseGfx();
    for (unsigned i = 0; i < numButtons; i++) {
        D1Gfx *gfx = gs->getGfx(i);
        this->buttons[i]->setToolTip(gfx->getFilePath());
        // this->buttons[i]->setDown(gfx == baseGfx);
        this->buttons[i]->setCheckable(gfx == baseGfx);
        this->buttons[i]->setChecked(gfx == baseGfx);
        this->buttons[i]->update();
    }

    { // update the components
    QComboBox *comboBox = this->ui->componentsComboBox;
    int prevIndex = comboBox->currentIndex();
    comboBox->hide();
    comboBox->clear();
    comboBox->addItem("", 0);
    count = this->gfx->getComponentCount();
    if (prevIndex < 0) {
        prevIndex = 0;
    } else if (count < prevIndex) {
        prevIndex = count;
    }
    for (int i = 0; i < count; i++) {
        D1GfxComp *comp = this->gfx->getComponent(i);
        QString labelText = comp->getLabel();
        if (comp->getGFX()->isModified()) {
            labelText += "*";
        }
        comboBox->addItem(labelText, i + 1);
    }
    comboBox->show();
    comboBox->setCurrentIndex(prevIndex);
    }

    // update the asset multiplier field
    this->ui->assetMplEdit->setText(QString::number(this->assetMpl));

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

    { // update gfxtype
        QComboBox *comboBox = this->ui->gfxTypeComboBox;
        comboBox->hide();
        comboBox->clear();
        for (int i = 0; i < 6; i++) {
            comboBox->addItem(D1Gfx::gfxTypeToStr((D1CEL_TYPE)i), i);
        }
        comboBox->show();
        comboBox->setCurrentIndex((int)this->gfx->getType());
    }
    { // update metatypes
        QComboBox *comboBox = this->ui->metaTypeComboBox;
        int prevIndex = std::max(0, comboBox->currentIndex());
        const QSize fs = this->gfx->getFrameSize();
        comboBox->hide();
        comboBox->clear();
        for (int i = 0; i < NUM_CELMETA; i++) {
            const D1GfxMeta *meta = this->gfx->getMeta(i);
            QString mark = "-";
            if (meta->isStored()) {
                mark = "+";
                if ((i == CELMETA_DIMENSIONS && !fs.isValid() && !this->ui->metaFrameDimensionsCheckBox->isChecked()) ||
                    (i == CELMETA_DIMENSIONS_PER_FRAME && this->gfx->getFrameCount() == 0))
                    mark = "?";
                if (i == CELMETA_ACTIONFRAMES || i == CELMETA_ANIMORDER) {
                    QList<int> result;
                    int num = D1Cel::parseFrameList(meta->getContent(), result);
                    if (result.isEmpty() || num != result.count()) {
                        mark = "?";
                    }
                }
            }
            comboBox->addItem(QString("%1 %2 %1").arg(mark).arg(D1GfxMeta::metaTypeToStr(i)), i);
        }
        comboBox->show();
        comboBox->setCurrentIndex(prevIndex);

        const bool isStored = this->gfx->getMeta(prevIndex)->isStored();
        this->ui->metaStoredCheckBox->setChecked(isStored);
        {
            bool isReadOnly = !this->ui->metaFrameDimensionsCheckBox->isChecked();
            if (this->gfx->getFrameCount() == 0 || !fs.isValid()) {
                isReadOnly = false;
            }
            this->ui->metaFrameWidthEdit->setReadOnly(isReadOnly);
            this->ui->metaFrameHeightEdit->setReadOnly(isReadOnly);
            this->ui->metaFrameHeightEdit->update();
            D1GfxMeta *meta = this->gfx->getMeta(CELMETA_DIMENSIONS);
            int w, h;
            if (isReadOnly) {
                w = fs.width();
                h = fs.height();
                bool change = meta->setWidth(w);
                change |= meta->setHeight(h);
                if (change && meta->isStored())
                    this->gfx->setModified();
            } else {
                w = meta->getWidth();
                h = meta->getHeight();
            }
            this->ui->metaFrameWidthEdit->setText(QString::number(w));
            this->ui->metaFrameHeightEdit->setText(QString::number(h));
        }
        {
            QString txt = this->gfx->getMeta(CELMETA_ANIMORDER)->getContent();
            if (txt != this->ui->animOrderEdit->toPlainText()) {
                this->ui->animOrderEdit->blockSignals(true);
                this->ui->animOrderEdit->setPlainText(txt);
                this->ui->animOrderEdit->blockSignals(false);
            }
        }
        {
            D1GfxMeta *meta = this->gfx->getMeta(CELMETA_ANIMDELAY);
            QString animDelay = meta->getContent();
            QCheckBox *cb = this->ui->metaAnimDelayCheckBox;
            Qt::CheckState mode = cb->checkState();
            bool isReadOnly = false;
            if (mode == Qt::Unchecked) {
                cb->setToolTip(tr("Set from the playback setting"));

                isReadOnly = true;
                int adInt = this->currentPlayDelay * SPEED_NORMAL / 1000000;
                animDelay = QString::number(adInt);
                this->gfx->setMetaContent(CELMETA_ANIMDELAY, animDelay);
            } else if (mode == Qt::PartiallyChecked) {
                cb->setToolTip(tr("Update the playback setting"));

                this->currentPlayDelay = animDelay.toInt() * (1000000 / SPEED_NORMAL);
            } else {
                cb->setToolTip(tr("Ignore the playback setting"));
            }
            this->ui->animDelayEdit->setReadOnly(isReadOnly);
            this->ui->animDelayEdit->setText(animDelay);
        }
        this->ui->actionFramesEdit->setText(this->gfx->getMeta(CELMETA_ACTIONFRAMES)->getContent());
    }

    // update clipped checkbox
    this->ui->celFramesClippedCheckBox->setChecked(this->gfx->isClipped());

    // set play-delay text
    this->ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));
}

CelScene *GfxsetView::getCelScene() const
{
    return const_cast<CelScene *>(&this->celScene);
}

int GfxsetView::getCurrentFrameIndex() const
{
    return this->currentFrameIndex;
}

void GfxsetView::framePixelClicked(const QPoint &pos, int flags)
{
    if (this->gfx->getFrameCount() == 0) {
        return;
    }

    QRect rect = this->gfx->getFrameRect(this->currentFrameIndex, true);
    QPoint p = pos;
    p -= QPoint(SET_SCENE_MARGIN, SET_SCENE_MARGIN);
    p += QPoint(rect.x(), rect.y());

    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx > 0) {
        D1GfxComp *comp = this->gfx->getComponent(compIdx - 1);
        D1GfxCompFrame *cFrame = comp->getCompFrame(this->currentFrameIndex);
        unsigned frameRef = cFrame->cfFrameRef;
        D1Gfx* cGfx = comp->getGFX();
        if (frameRef != 0 && frameRef <= (unsigned)cGfx->getFrameCount()) {
            p -= QPoint(cFrame->cfOffsetX, cFrame->cfOffsetY);

            D1GfxFrame *frame = cGfx->getFrame(frameRef - 1);
            dMainWindow().frameClicked(frame, p, flags);
        }
        return;
    }

    D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
    dMainWindow().frameClicked(frame, p, flags);
}

bool GfxsetView::framePos(QPoint &pos) const
{
    if (this->gfx->getFrameCount() != 0) {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        pos -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        return pos.x() >= 0 && pos.x() < frame->getWidth() && pos.y() >= 0 && pos.y() < frame->getHeight();
    }

    return false;
}

void GfxsetView::framePixelHovered(const QPoint &pos) const
{
    QPoint tpos = pos;
    if (this->framePos(tpos)) {
        dMainWindow().pointHovered(tpos);
        return;
    }

    tpos.setX(UINT_MAX);
    tpos.setY(UINT_MAX);
    dMainWindow().pointHovered(tpos);
}

void GfxsetView::createFrame(bool append)
{
    int width, height;
    int frameCount = this->gfx->getFrameCount();
    if (frameCount != 0) {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        width = frame->getWidth();
        height = frame->getHeight();
    } else {
        // TODO: request from the user?
        width = 64;
        height = 128;
    }
    int newFrameIndex = append ? this->gfx->getFrameCount() : this->currentFrameIndex;
    this->gfx->insertFrame(newFrameIndex, width, height);
    // jump to the new frame
    this->currentFrameIndex = newFrameIndex;
    // update the view - done by the caller
    // this->displayFrame();
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
        bool palMod;
        D1GfxFrame *frame = this->gfx->insertFrame(index);
        if (!D1Pcx::load(*frame, imagefilePath, this->pal, this->gfx->getPalette(), &palMod)) {
            this->gfx->removeFrame(index, false);
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
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
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

void GfxsetView::duplicateCurrentFrame(bool wholeGroup)
{
    this->currentFrameIndex = this->gfx->duplicateFrame(this->currentFrameIndex, wholeGroup);

    this->updateGroupIndex();
    // update the view
    this->updateFields();
}

void GfxsetView::replaceCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame *frame = new D1GfxFrame();
        bool success = D1Pcx::load(*frame, imagefilePath, this->pal, this->gfx->getPalette(), &palMod);
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

void GfxsetView::removeCurrentFrame(bool wholeGroup)
{
    // remove the frame
    this->gfx->removeFrame(this->currentFrameIndex, wholeGroup);
    int numFrame = this->gfx->getFrameCount();
    if (numFrame <= this->currentFrameIndex) {
        this->currentFrameIndex = std::max(0, numFrame - 1);
    }
    this->updateGroupIndex();
    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::flipHorizontalCurrentFrame(bool wholeGroup)
{
    // flip the frame
    this->gfx->flipHorizontalFrame(this->currentFrameIndex, wholeGroup);

    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::flipVerticalCurrentFrame(bool wholeGroup)
{
    // flip the frame
    this->gfx->flipVerticalFrame(this->currentFrameIndex, wholeGroup);

    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::mergeFrames(const MergeFramesParam &params)
{
    int firstFrameIdx = params.rangeFrom;
    int lastFrameIdx = params.rangeTo;
    // assert(firstFrameIdx >= 0 && firstFrameIdx < lastFrameIdx && lastFrameIdx < this->gfx->getFrameCount());
    this->gfx->mergeFrames(firstFrameIdx, lastFrameIdx);
    if (this->currentFrameIndex > firstFrameIdx) {
        if (this->currentFrameIndex <= lastFrameIdx) {
            this->currentFrameIndex = firstFrameIdx;
        } else {
            this->currentFrameIndex -= lastFrameIdx - firstFrameIdx;
        }
    }
    this->updateGroupIndex();
    // update the view - done by the caller
    // this->displayFrame();
}

QString GfxsetView::copyCurrentPixels(bool values) const
{
    if (this->gfx->getFrameCount() == 0) {
        return QString();
    }
    return this->gfx->getFramePixels(this->currentFrameIndex, values);
}

void GfxsetView::pasteCurrentPixels(const QString &pixels)
{
    if (this->gfx->getFrameCount() != 0) {
        this->gfx->replaceFrame(this->currentFrameIndex, pixels);
    } else {
        this->gfx->insertFrame(this->currentFrameIndex, pixels);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

QImage GfxsetView::copyCurrentImage() const
{
    if (this->gfx->getFrameCount() == 0) {
        return QImage();
    }
    return this->gfx->getFrameImage(this->currentFrameIndex);
}

void GfxsetView::pasteCurrentImage(const QImage &image)
{
    if (this->gfx->getFrameCount() != 0) {
        this->gfx->replaceFrame(this->currentFrameIndex, image);
    } else {
        this->gfx->insertFrame(this->currentFrameIndex, image);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void GfxsetView::coloredFrames(bool gfxOnly, const std::pair<int, int>& colors) const
{
    ProgressDialog::incBar(gfxOnly ? tr("Checking frames...") : tr("Checking graphics..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    if ((unsigned)colors.first >= D1PAL_COLORS) {
        progress.second = tr("Frames with transparent pixels:");
    } else {
        progress.second = tr("Frames with pixels in the [%1..%2] color range:").arg(colors.first).arg(colors.second);
    }

    dProgress() << progress;
    for (int gn = 0; gn < this->gfxset->getGfxCount(); gn++) {
        D1Gfx *gfx = this->gfxset->getGfx(gn);
        if (gfxOnly && gfx != this->gfx)
            continue;
        for (int i = 0; i < gfx->getFrameCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = gfx->getFramePixelImage(i);
            int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
            if (numPixels != 0) {
                dProgress() << tr("Frame %1 of %2 has %n affected pixels.", "", numPixels).arg(i + 1).arg(this->gfxset->getGfxLabel(gn));
                result = true;
            }
        }
    }

    if (!result) {
        if ((unsigned)colors.first >= D1PAL_COLORS) {
            progress.second = tr("None of the frames have transparent pixel.");
        } else {
            progress.second = tr("None of the frames are using the colors [%1..%2].").arg(colors.first).arg(colors.second);
        }
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void GfxsetView::activeFrames(bool gfxOnly) const
{
    ProgressDialog::incBar(gfxOnly ? tr("Checking frames...") : tr("Checking graphics..."), 1);
    QComboBox *cycleBox = this->ui->playComboBox;
    QString cycleTypeTxt = cycleBox->currentText();
    int cycleType = cycleBox->currentIndex();
    if (cycleType != 0) {
        int cycleColors = D1Pal::getCycleColors((D1PAL_CYCLE_TYPE)(cycleType - 1));
        const std::pair<int, int> colors = { 1, cycleColors - 1 };
        bool result = false;

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Active frames (using '%1' playback mode):").arg(cycleTypeTxt);

        dProgress() << progress;
        for (int gn = 0; gn < this->gfxset->getGfxCount(); gn++) {
            D1Gfx *gfx = this->gfxset->getGfx(gn);
            if (gfxOnly && gfx != this->gfx)
                continue;
            for (int i = 0; i < gfx->getFrameCount(); i++) {
                const std::vector<std::vector<D1GfxPixel>> pixelImage = gfx->getFramePixelImage(i);
                int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
                if (numPixels != 0) {
                    dProgress() << tr("Frame %1 of %2 has %n affected pixels.", "", numPixels).arg(i + 1).arg(this->gfxset->getGfxLabel(gn));
                    result = true;
                }
            }
        }

        if (!result) {
            progress.second = tr("None of the frames are active in '%1' playback mode.").arg(cycleTypeTxt);
            dProgress() << progress;
        }
    } else {
        dProgress() << tr("Colors are not affected if the playback mode is '%1'.").arg(cycleTypeTxt);
    }

    ProgressDialog::decBar();
}

void GfxsetView::checkGraphics(bool gfxOnly) const
{
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Inconsistencies in the graphics of the gfx-set:");

    dProgress() << progress;
    result = this->gfxset->check(gfxOnly ? this->gfx : nullptr, this->assetMpl);

    if (!result) {
        progress.second = gfxOnly ? tr("No inconsistency detected in the current gfx.") : tr("No inconsistency detected in the gfx-set.");
        dProgress() << progress;
    }
}

void GfxsetView::resize(const ResizeParam &params)
{
    int frameWithPixelLost = -1;
    int gn = 0;
    for ( ; gn < this->gfxset->getGfxCount(); gn++) {
        D1Gfx *gfx = this->gfxset->getGfx(gn);
        if (!params.resizeAll && gfx != this->gfx) {
            continue;
        }
        frameWithPixelLost = gfx->testResize(params);
        if (frameWithPixelLost != -1) {
            break;
        }
    }
    if (frameWithPixelLost != -1) {
        QMessageBox::StandardButton reply;
        QString frameId = tr("Frame %1 of %2").arg(frameWithPixelLost + 1).arg(this->gfxset->getGfxLabel(gn));
        reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Pixels with non-background colors are going to be eliminated (At least %1 is affected). Are you sure you want to proceed?").arg(frameId), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Resizing..."), 1, PAF_NONE);

    bool ch = false;
    for (int gn = 0; gn < this->gfxset->getGfxCount(); gn++) {
        D1Gfx *gfx = this->gfxset->getGfx(gn);
        if (!params.resizeAll && gfx != this->gfx) {
            continue;
        }
        bool change = gfx->resize(params);
        if (change) {
            gfx->setModified();
            ch = true;
        }
    }
    ProgressDialog::done();

    if (ch) {
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

void GfxsetView::drawGrid(QImage &celFrame)
{
    int width = celFrame.width();
    int height = celFrame.height();
    QColor color = this->pal->getUndefinedColor();

    unsigned microHeight = MICRO_HEIGHT * this->assetMpl;
    for (int i = (height + microHeight) / microHeight - 1; i >= 0; i--) {
        for (int x = 0; x < width; x++) {
            int y0 = height - microHeight * (i + 1) + ( 0 + (x - width / 2) / 2) % microHeight;
            if (y0 >= 0 && y0 < height)
                celFrame.setPixelColor(x, y0, color);

            int y1 = height - microHeight * (i + 1) + (microHeight - 1 - (x - width / 2) / 2) % microHeight;
            if (y1 >= 0 && y1 < height)
                celFrame.setPixelColor(x, y1, color);
        }
    }
}

void GfxsetView::displayFrame()
{
    this->updateFields();

    this->celScene.clear();

    // Getting the current frame to display
    Qt::CheckState compType = this->ui->showComponentsCheckBox->checkState();
    int component = compType == Qt::Unchecked ? 0 : (compType == Qt::PartiallyChecked ? this->ui->componentsComboBox->currentIndex() : -1);
    const int outline = this->ui->showOutlineCheckBox->isChecked() ? this->selectedColor : -1;
    QImage celFrame = this->gfx->getFrameCount() != 0 ? this->gfx->getFrameImage(this->currentFrameIndex, component, outline) : QImage();

    // add grid if requested
    if (this->ui->showGridCheckBox->isChecked()) {
        this->drawGrid(celFrame);
    }

    this->celScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32);
    celFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->celScene.setSceneRect(0, 0,
        SET_SCENE_MARGIN + celFrame.width() + SET_SCENE_MARGIN,
        SET_SCENE_MARGIN + celFrame.height() + SET_SCENE_MARGIN);
    // ui->celGraphicsView->adjustSize();

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene.addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(SET_SCENE_MARGIN, SET_SCENE_MARGIN);
    this->celScene.addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(SET_SCENE_MARGIN, SET_SCENE_MARGIN);

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

void GfxsetView::changeColor(const QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements, bool all)
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
    QAction actions[9];

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    // 'Frame' submenu of 'Edit'
    int cursor = 0;
    actions[cursor].setText(tr("Merge"));
    actions[cursor].setToolTip(tr("Merge frames of the current graphics"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionMerge_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Add Layer"));
    actions[cursor].setToolTip(tr("Add the content of an image to the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddTo_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    contextMenu.addSeparator();

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add new frames before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Duplicate"));
    actions[cursor].setToolTip(tr("Duplicate the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDuplicate_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Append"));
    actions[cursor].setToolTip(tr("Append new frames at the end"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAdd_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Replace"));
    actions[cursor].setToolTip(tr("Replace the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionReplace_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Del"));
    actions[cursor].setToolTip(tr("Delete the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDel_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    contextMenu.addSeparator();

    cursor++;
    actions[cursor].setText(tr("Horizontal Flip"));
    actions[cursor].setToolTip(tr("Flip the current frame horizontally"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionFlipHorizontal_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Vertical Flip"));
    actions[cursor].setToolTip(tr("Flip the current frame vertically"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionFlipVertical_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    contextMenu.exec(mapToGlobal(pos));
}

void GfxsetView::selectGfx(int gfxIndex)
{
    D1Gfx* nextGfx = this->gfxset->getGfx(gfxIndex);
    // preserve group-index (and relative frame-index of the group if possible)
    D1Gfx* currGfx = this->gfx;
    if (this->ui->framesGroupCheckBox->isChecked() && this->currentGroupIndex < currGfx->getGroupCount() && currGfx->getGroupCount() == nextGfx->getGroupCount()) {
        int frameIndex = this->currentFrameIndex - currGfx->getGroupFrameIndices(this->currentGroupIndex).first;
        this->currentFrameIndex = nextGfx->getGroupFrameIndices(this->currentGroupIndex).first + frameIndex;
    }

    dMainWindow().gfxChanged(nextGfx);
}

void GfxsetView::palColorsSelected(const std::vector<quint8> &indices)
{
    if (indices.empty()) {
        this->selectedColor = -1;
    } else {
        this->selectedColor = indices.front();
    }
    this->displayFrame();
}

void GfxsetView::on_misNWButton_clicked()
{
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 6 : DIR_NW);
}

void GfxsetView::on_misNNWButton_clicked()
{
    this->selectGfx(7);
}

void GfxsetView::on_misNButton_clicked()
{
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 8 : DIR_N);
}

void GfxsetView::on_misNNEButton_clicked()
{
    this->selectGfx(9);
}

void GfxsetView::on_misNEButton_clicked()
{
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 10 : DIR_NE);
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
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 4 : DIR_W);
}

void GfxsetView::on_misEButton_clicked()
{
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 12 : DIR_E);
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
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 2 : DIR_SW);
}

void GfxsetView::on_misSSWButton_clicked()
{
    this->selectGfx(1);
}

void GfxsetView::on_misSButton_clicked()
{
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 0 : DIR_S);
}

void GfxsetView::on_misSSEButton_clicked()
{
    this->selectGfx(15);
}

void GfxsetView::on_misSEButton_clicked()
{
    this->selectGfx(this->gfxset->getGfxCount() == 16 ? 14 : DIR_SE);
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

void GfxsetView::on_newComponentPushButtonClicked()
{
    QString gfxFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)"));
    if (gfxFilePath.isEmpty()) {
        return;
    }

    D1Gfx* gfx = this->gfx->loadComponentGFX(gfxFilePath);
    if (gfx == nullptr) {
        return;
    }

    int compIdx = this->ui->componentsComboBox->currentIndex();
    
    this->gfx->insertComponent(compIdx, gfx);

    dMainWindow().updateWindow();
}

void GfxsetView::on_editComponentPushButtonClicked()
{
    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx != 0 && this->gfx->getFrameCount() != 0) {
        if (this->gfxComponentDialog == nullptr) {
            this->gfxComponentDialog = new GfxComponentDialog(this);
        }
        this->gfxComponentDialog->initialize(this->gfx, this->gfx->getComponent(compIdx - 1));
        this->gfxComponentDialog->show();
    }
}


void GfxsetView::on_reloadComponentPushButtonClicked()
{
    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx != 0) {
        D1GfxComp *gfxComp = this->gfx->getComponent(compIdx - 1);
        D1Gfx* cGfx = this->gfx->loadComponentGFX(gfxComp->getGFX()->getFilePath());
        if (cGfx != nullptr) {
            gfxComp->setGFX(cGfx);

            this->displayFrame();
        }
    }
}

void GfxsetView::on_closeComponentPushButtonClicked()
{
    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx != 0) {
        this->gfx->removeComponent(compIdx - 1);

        dMainWindow().updateWindow();
    }
}

void GfxsetView::on_componentsComboBox_activated(int index)
{
    // redraw the frame
    this->displayFrame();
}

void GfxsetView::on_showComponentsCheckBox_clicked()
{
    // redraw the frame
    this->displayFrame();
}

void GfxsetView::on_framesGroupCheckBox_clicked()
{
    // update frameIndexEdit and frameNumberEdit
    this->updateFields();
}

void GfxsetView::on_firstFrameButton_clicked()
{
    int nextFrameIndex = 0;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
        if (moveFrame) {
            for (int i = this->currentFrameIndex; i > nextFrameIndex; i--) {
                this->gfx->swapFrames(i, i - 1);
            }
        }
    } else {
        if (moveFrame) {
            this->gfx->swapFrames(UINT_MAX, this->currentFrameIndex);
        }
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxsetView::on_previousFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex - 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::max(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).first);
    }
    if (moveFrame) {
        this->gfx->swapFrames(nextFrameIndex, this->currentFrameIndex);
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxsetView::on_nextFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex + 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::min(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).second);
    }
    if (moveFrame) {
        this->gfx->swapFrames(this->currentFrameIndex, nextFrameIndex);
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxsetView::on_lastFrameButton_clicked()
{
    int nextFrameIndex = this->gfx->getFrameCount() - 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).second;
        if (moveFrame) {
            for (int i = this->currentFrameIndex; i < nextFrameIndex; i++) {
                this->gfx->swapFrames(i, i + 1);
            }
        }
    } else {
        if (moveFrame) {
            this->gfx->swapFrames(this->currentFrameIndex, UINT_MAX);
        }
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
    this->updateFields();
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
    this->updateFields();
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

void GfxsetView::on_showGridCheckBox_clicked()
{
    this->displayFrame();
}

void GfxsetView::on_showOutlineCheckBox_clicked()
{
    this->displayFrame();
}

void GfxsetView::on_assetMplEdit_returnPressed()
{
    unsigned ampl = this->ui->assetMplEdit->text().toUShort();
    if (ampl == 0)
        ampl = 1;
    if (this->assetMpl != ampl) {
        this->assetMpl = ampl;
        this->displayFrame();
    }

    this->on_assetMplEdit_escPressed();
}

void GfxsetView::on_assetMplEdit_escPressed()
{
    this->updateFields();
    this->ui->assetMplEdit->clearFocus();
}

void GfxsetView::on_celFramesClippedCheckBox_clicked()
{
    this->gfx->setClipped(this->ui->celFramesClippedCheckBox->isChecked());
    this->updateFields();
}

void GfxsetView::on_actionToggle_Mode_triggered()
{
    this->ui->mainStackedLayout->setCurrentIndex(1 - this->ui->mainStackedLayout->currentIndex());
}

void GfxsetView::on_metaTypeComboBox_activated(int index)
{
    this->ui->metaTypeStackedLayout->setCurrentIndex(index);

    this->updateFields();
}

void GfxsetView::on_metaStoredCheckBox_clicked()
{
    D1GfxMeta *meta = this->gfx->getMeta(this->ui->metaTypeStackedLayout->currentIndex());
    meta->setStored(!meta->isStored());
    this->gfx->setModified();

    this->updateFields();
}

void GfxsetView::on_metaFrameWidthEdit_returnPressed()
{
    QString text = this->ui->metaFrameWidthEdit->text();

    D1GfxMeta *meta = this->gfx->getMeta(CELMETA_DIMENSIONS);
    if (meta->setWidth(text.toInt()) && meta->isStored())
        this->gfx->setModified();

    this->on_metaFrameWidthEdit_escPressed();
}

void GfxsetView::on_metaFrameWidthEdit_escPressed()
{
    // update metaFrameWidthEdit
    this->updateFields();
    this->ui->metaFrameWidthEdit->clearFocus();
}

void GfxsetView::on_metaFrameHeightEdit_returnPressed()
{
    QString text = this->ui->metaFrameHeightEdit->text();

    D1GfxMeta *meta = this->gfx->getMeta(CELMETA_DIMENSIONS);
    if (meta->setHeight(text.toInt()) && meta->isStored())
        this->gfx->setModified();

    this->on_metaFrameHeightEdit_escPressed();
}

void GfxsetView::on_metaFrameHeightEdit_escPressed()
{
    // update metaFrameHeightEdit
    this->updateFields();
    this->ui->metaFrameHeightEdit->clearFocus();
}

void GfxsetView::on_metaFrameDimensionsCheckBox_clicked()
{
    this->updateFields();
}

void GfxsetView::on_animOrderEdit_textChanged()
{
    QString text = this->ui->animOrderEdit->toPlainText();

    this->gfx->setMetaContent(CELMETA_ANIMORDER, text);

    this->updateFields();
}

void GfxsetView::on_formatAnimOrderButton_clicked()
{
    QString text = this->ui->animOrderEdit->toPlainText();

    D1Cel::formatFrameList(text);

    this->ui->animOrderEdit->setPlainText(text);

    // this->on_animOrderEdit_textChanged();
}

void GfxsetView::on_animDelayEdit_returnPressed()
{
    QString text = this->ui->animDelayEdit->text();

    this->gfx->setMetaContent(CELMETA_ANIMDELAY, text);

    this->on_animDelayEdit_escPressed();
}

void GfxsetView::on_animDelayEdit_escPressed()
{
    // update actionFramesEdit
    this->updateFields();
    this->ui->animDelayEdit->clearFocus();
}

void GfxsetView::on_metaAnimDelayCheckBox_clicked()
{
    this->updateFields();
}

void GfxsetView::on_actionFramesEdit_returnPressed()
{
    QString text = this->ui->actionFramesEdit->text();

    this->gfx->setMetaContent(CELMETA_ACTIONFRAMES, text);

    this->on_actionFramesEdit_escPressed();
}

void GfxsetView::on_actionFramesEdit_escPressed()
{
    // update actionFramesEdit
    this->updateFields();
    this->ui->actionFramesEdit->clearFocus();
}

void GfxsetView::on_formatActionFramesButton_clicked()
{
    QString text = this->ui->actionFramesEdit->text();

    D1Cel::formatFrameList(text);

    this->ui->actionFramesEdit->setText(text);

    this->on_actionFramesEdit_returnPressed();
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
    unsigned playDelay = this->ui->playDelayEdit->text().toUInt();

    if (playDelay != 0)
        this->currentPlayDelay = playDelay;

    this->on_playDelayEdit_escPressed();
}

void GfxsetView::on_playDelayEdit_escPressed()
{
    // update playDelayEdit
    this->updateFields();
    this->ui->playDelayEdit->clearFocus();
}

void GfxsetView::on_playStopButton_clicked()
{
    if (this->playTimer != 0) {
        this->killTimer(this->playTimer);
        this->playTimer = 0;

        // restore the currentFrameIndex
        this->currentFrameIndex = this->origFrameIndex;
        // update group-index because the user might have changed it in the meantime
        this->updateGroupIndex();
        // restore palette
        dMainWindow().resetPaletteCycle();
        // change the label of the button
        this->ui->playStopButton->setText(tr("Play"));
        // enable the related fields
        this->ui->playDelayEdit->setReadOnly(false);
        this->ui->playComboBox->setEnabled(true);
        this->ui->playFrameCheckBox->setEnabled(true);
        this->ui->animOrderEdit->setReadOnly(false);
        return;
    }
    // disable the related fields
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(false);
    this->ui->playFrameCheckBox->setEnabled(false);
    this->ui->animOrderEdit->setReadOnly(true);
    // change the label of the button
    this->ui->playStopButton->setText(tr("Stop"));
    // preserve the currentFrameIndex
    this->origFrameIndex = this->currentFrameIndex;
    // preserve the palette
    dMainWindow().initPaletteCycle();
    // initialize animation-order
    D1GfxMeta *meta = this->gfx->getMeta(CELMETA_ANIMORDER);
    this->animOrder.clear();
    D1Cel::parseFrameList(meta->getContent(), this->animOrder);
    for (auto it = this->animOrder.begin(); it != this->animOrder.end(); ) {
        if (*it < this->gfx->getFrameCount()) {
            it++;
        } else {
            it = this->animOrder.erase(it);
        }
    }
    this->animFrameIndex = 0;

    this->playTimer = this->startTimer(this->currentPlayDelay / 1000, Qt::PreciseTimer);
}

void GfxsetView::on_playReverseCheckBox_clicked()
{
    this->animAdd = -this->animAdd;
}

void GfxsetView::timerEvent(QTimerEvent *event)
{
    if (this->gfx->getGroupCount() == 0) {
        return;
    }
    std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);

    int nextFrameIndex;
    if (!this->animOrder.isEmpty()) {
        int currAnimFrameIndex = this->animFrameIndex;
        if (currAnimFrameIndex >= this->animOrder.count())
            currAnimFrameIndex = 0;
        nextFrameIndex = this->animOrder[currAnimFrameIndex];
        this->animFrameIndex = currAnimFrameIndex + 1;
    } else {
        nextFrameIndex = this->currentFrameIndex + this->animAdd;
        Qt::CheckState playType = this->ui->playFrameCheckBox->checkState();
        if (playType == Qt::Unchecked) {
            // normal playback
            if (nextFrameIndex > groupFrameIndices.second)
                nextFrameIndex = groupFrameIndices.first;
            else if (nextFrameIndex < groupFrameIndices.first)
                nextFrameIndex = groupFrameIndices.second;
        } else if (playType == Qt::PartiallyChecked) {
            // playback till the original frame
            if (nextFrameIndex > this->origFrameIndex)
                nextFrameIndex = groupFrameIndices.first;
            else if (nextFrameIndex < groupFrameIndices.first)
                nextFrameIndex = this->origFrameIndex;
        } else {
            // playback from the original frame
            if (nextFrameIndex > groupFrameIndices.second)
                nextFrameIndex = this->origFrameIndex;
            else if (nextFrameIndex < this->origFrameIndex)
                nextFrameIndex = groupFrameIndices.second;
        }
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
