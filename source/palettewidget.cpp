#include "palettewidget.h"

#include <algorithm>
#include <vector>

#include <QClipboard>
#include <QColorDialog>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMimeData>

#include "config.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_palettewidget.h"

#define COLORIDX_TRANSPARENT -1

enum class COLORFILTER_TYPE {
    NONE,
    USED,
    TILE,
    SUBTILE,
    FRAME,
    TRANSLATED,
};

Q_DECLARE_METATYPE(COLORFILTER_TYPE)

EditPaletteCommand::EditPaletteCommand(D1Pal *p, quint8 sci, quint8 eci, QColor nc, QColor ec)
    : QUndoCommand(nullptr)
    , pal(p)
    , startColorIndex(sci)
    , endColorIndex(eci)
{
    float step = 1.0f / (eci - sci + 1);

    for (int i = sci; i <= eci; i++) {
        float factor = (i - sci) * step;

        QColor color(
            nc.red() * (1 - factor) + ec.red() * factor,
            nc.green() * (1 - factor) + ec.green() * factor,
            nc.blue() * (1 - factor) + ec.blue() * factor);

        this->modColors.push_back(color);
    }
}

EditPaletteCommand::EditPaletteCommand(D1Pal *p, quint8 sci, quint8 eci, const QList<QColor> &mc)
    : QUndoCommand(nullptr)
    , pal(p)
    , startColorIndex(sci)
    , endColorIndex(eci)
    , modColors(mc)
{
}

void EditPaletteCommand::undo()
{
    if (this->pal.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (int i = startColorIndex; i <= endColorIndex; i++) {
        QColor palColor = this->pal->getColor(i);
        this->pal->setColor(i, this->modColors[i - startColorIndex]);
        this->modColors[i - startColorIndex] = palColor;
    }

    emit this->modified();
}

void EditPaletteCommand::redo()
{
    this->undo();
}

EditTranslationCommand::EditTranslationCommand(D1Trn *t, quint8 startColorIndex, quint8 endColorIndex, const std::vector<quint8> *nt)
    : QUndoCommand(nullptr)
    , trn(t)
{
    for (quint8 i = startColorIndex; i <= endColorIndex; i++) {
        this->modTranslations.push_back(std::pair<quint8, quint8>(i, nt == nullptr ? i : (*nt)[i - startColorIndex]));
    }
}

EditTranslationCommand::EditTranslationCommand(D1Trn *t, const std::vector<std::pair<quint8, quint8>> &mt)
    : QUndoCommand(nullptr)
    , trn(t)
    , modTranslations(mt)
{
}

void EditTranslationCommand::undo()
{
    if (this->trn.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (std::pair<quint8, quint8> &mod : this->modTranslations) {
        quint8 trnValue = this->trn->getTranslation(mod.first);
        this->trn->setTranslation(mod.first, mod.second);
        mod.second = trnValue;
    }

    emit this->modified();
}

void EditTranslationCommand::redo()
{
    this->undo();
}

PaletteScene::PaletteScene(PaletteWidget *v)
    : QGraphicsScene(0, 0, PALETTE_WIDTH, PALETTE_WIDTH, v)
    , view(v)
{
    // Setting background color
    this->setBackgroundBrush(Qt::white);
}

int PaletteScene::getColorIndexFromCoordinates(QPointF coordinates)
{
    int index = 0;

    int w = PALETTE_WIDTH / PALETTE_COLORS_PER_LINE;

    int ix = coordinates.x() / w;
    int iy = coordinates.y() / w;

    if (ix < 0) {
        ix = 0;
    }
    if (ix >= PALETTE_COLORS_PER_LINE) {
        ix = PALETTE_COLORS_PER_LINE - 1;
    }
    if (iy < 0) {
        iy = 0;
    }
    if (iy >= D1PAL_COLORS / PALETTE_COLORS_PER_LINE) {
        iy = D1PAL_COLORS / PALETTE_COLORS_PER_LINE - 1;
    }
    index = iy * PALETTE_COLORS_PER_LINE + ix;

    return index;
}

QRectF PaletteScene::getColorCoordinates(quint8 index)
{
    int ix = index % PALETTE_COLORS_PER_LINE;
    int iy = index / PALETTE_COLORS_PER_LINE;

    int w = PALETTE_WIDTH / PALETTE_COLORS_PER_LINE;

    QRectF coordinates(ix * w, iy * w, w, w);

    return coordinates;
}

void PaletteScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    QPointF pos = event->scenePos();

    qDebug() << QStringLiteral("Clicked: %1:%2").arg(pos.x()).arg(pos.y());

    // Check if selected color has changed
    int colorIndex = PaletteScene::getColorIndexFromCoordinates(pos);

    // if (colorIndex >= 0)
    this->view->startColorSelection(colorIndex);
}

void PaletteScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    QPointF pos = event->scenePos();

    // Check if selected color has changed
    int colorIndex = PaletteScene::getColorIndexFromCoordinates(pos);

    // if (colorIndex >= 0)
    this->view->changeColorSelection(colorIndex);
}

void PaletteScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    this->view->finishColorSelection();
}

void PaletteScene::keyPressEvent(QKeyEvent *event)
{
    int dir = event->key();
    if (dir != Qt::Key_Left && dir != Qt::Key_Right && dir != Qt::Key_Up && dir != Qt::Key_Down) {
        QGraphicsScene::keyPressEvent(event);
        return;
    }
    QMessageBox::critical(this, "Error", "scene key event");
    bool extend = (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) != 0;
    this->view->changeColorSelection(dir, extend);
}

void PaletteScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    this->dragMoveEvent(event);
}

void PaletteScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    bool isTrn = ((PaletteWidget *)this->view)->isTrnWidget();
    const char *ext = isTrn ? ".trn" : ".pal";
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.toLocalFile().toLower().endsWith(ext)) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void PaletteScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    bool isTrn = ((PaletteWidget *)this->view)->isTrnWidget();
    const char *ext = isTrn ? ".trn" : ".pal";
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.toLocalFile().toLower().endsWith(ext)) {
            filePaths.append(url.toLocalFile());
        }
    }
    // try to insert pal/trn files
    dMainWindow().openPalFiles(filePaths, (PaletteWidget *)this->view);
}

PaletteWidget::PaletteWidget(QWidget *parent, QUndoStack *us, QString title)
    : QWidget(parent)
    , undoStack(us)
    , ui(new Ui::PaletteWidget())
{
    this->ui->setupUi(this);
    this->ui->graphicsView->setScene(&this->scene);
    this->ui->headerLabel->setText(title);

    // add icon-buttons
    QLayout *layout = this->ui->headerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("New"), this, &PaletteWidget::on_newPushButtonClicked); // use SP_FileIcon ?
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogOpenButton, tr("Open"), this, &PaletteWidget::on_openPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogSaveButton, tr("Save"), this, &PaletteWidget::on_savePushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogSaveButton, tr("Save As"), this, &PaletteWidget::on_saveAsPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &PaletteWidget::on_closePushButtonClicked); // use SP_DialogDiscardButton ?

    // adjust the size of the push-buttons
    QFontMetrics fm = this->fontMetrics();
    QPushButton *pickBtn = this->ui->colorPickPushButton;
    QPushButton *clearBtn = this->ui->colorClearPushButton;
    // - calculate the border
    QSize pickSize = fm.size(Qt::TextShowMnemonic, pickBtn->text());
    QStyleOptionButton opt;
    opt.initFrom(pickBtn);
    int pickWidth = pickBtn->style()->sizeFromContents(QStyle::CT_PushButton, &opt, pickSize, pickBtn).width();
    int border = pickWidth - pickSize.width();
    // - calculate the width of the other buttons
    constexpr int spacing = 4;
    int clearWidth = fm.size(Qt::TextShowMnemonic, clearBtn->text()).width() + border;
    // - select appropriate width
    int colorWidth = std::max(pickWidth, clearWidth);
    int btnsWidth = 2 * colorWidth + spacing;
    colorWidth = (btnsWidth - spacing) / 2;
    // - set the calculated widths
    pickBtn->setMinimumWidth(colorWidth);
    pickBtn->setMaximumWidth(colorWidth);
    clearBtn->setMinimumWidth(colorWidth);
    clearBtn->setMaximumWidth(colorWidth);

    // connect esc events of LineEditWidget
    QObject::connect(this->ui->colorLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_colorLineEdit_escPressed()));
    QObject::connect(this->ui->translationIndexLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_translationIndexLineEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));
}

PaletteWidget::~PaletteWidget()
{
    delete ui;
}

void PaletteWidget::setPal(D1Pal *p)
{
    this->pal = p;

    // this->refreshPathComboBox();

    this->modify();
}

void PaletteWidget::setTrn(D1Trn *t)
{
    this->trn = t;

    // this->refreshPathComboBox();

    this->modify();
}

bool PaletteWidget::isTrnWidget()
{
    return this->isTrn;
}

void PaletteWidget::initialize(D1Pal *p, CelView *cv, LevelCelView *lcv, GfxsetView *gsv, D1PalHits *ph)
{
    this->isTrn = false;
    this->pal = p;
    this->trn = nullptr;
    this->celView = cv;
    this->levelCelView = lcv;
    this->gfxsetView = gsv;
    this->palHits = ph;

    this->initializeUi();
}

void PaletteWidget::initialize(D1Trn *t, CelView *cv, LevelCelView *lcv, GfxsetView *gsv, D1PalHits *ph)
{
    this->isTrn = true;
    this->pal = nullptr;
    this->trn = t;
    this->celView = cv;
    this->levelCelView = lcv;
    this->gfxsetView = gsv;
    this->palHits = ph;

    this->initializeUi();
}

void PaletteWidget::initializeUi()
{
    bool trnMode = this->isTrn;

    this->ui->translationIndexSep->setVisible(trnMode);
    this->ui->translationIndexLineEdit->setVisible(trnMode);

    // this->initializePathComboBox();
    this->initializeDisplayComboBox();
}

void PaletteWidget::initializeDisplayComboBox()
{
    ui->displayComboBox->addItem(tr("Show all colors"), QVariant::fromValue(COLORFILTER_TYPE::NONE));

    if (!this->isTrn) {
        ui->displayComboBox->addItem(tr("Show all frames hits"), QVariant::fromValue(COLORFILTER_TYPE::USED));
        if (this->levelCelView != nullptr) {
            ui->displayComboBox->addItem(tr("Show current tile hits"), QVariant::fromValue(COLORFILTER_TYPE::TILE));
            ui->displayComboBox->addItem(tr("Show current subtile hits"), QVariant::fromValue(COLORFILTER_TYPE::SUBTILE));
        }
        ui->displayComboBox->addItem(tr("Show current frame hits"), QVariant::fromValue(COLORFILTER_TYPE::FRAME));
    } else {
        ui->displayComboBox->addItem(tr("Show the altered colors"), QVariant::fromValue(COLORFILTER_TYPE::TRANSLATED));
    }
}

void PaletteWidget::selectColor(const D1GfxPixel &pixel)
{
    this->initStopColorPicking();

    int index;

    if (pixel.isTransparent()) {
        index = COLORIDX_TRANSPARENT;
    } else {
        index = pixel.getPaletteIndex();
    }
    this->selectedFirstColorIndex = index;
    this->selectedLastColorIndex = index;

    this->updateFields();
}

std::pair<int, int> PaletteWidget::getCurrentSelection() const
{
    return std::pair<int, int>(this->selectedFirstColorIndex, this->selectedLastColorIndex);
}

void PaletteWidget::checkTranslationsSelection(const std::vector<quint8> &indexes)
{
    // assert(this->selectedLastColorIndex != COLORIDX_TRANSPARENT);
    unsigned selectionLength = this->selectedLastColorIndex - this->selectedFirstColorIndex + 1;
    if (selectionLength != indexes.size()) {
        QMessageBox::warning(this, tr("Warning"), tr("Source and target selection length do not match."));
        return;
    }

    // Build color editing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    EditTranslationCommand *command = new EditTranslationCommand(
        this->trn, this->selectedFirstColorIndex, this->selectedLastColorIndex, &indexes);
    QObject::connect(command, &EditTranslationCommand::modified, this, &PaletteWidget::modify);

    this->undoStack->push(command);

    emit this->colorPicking_stopped(); // finish color picking
}

QList<QPair<int, QColor>> clipboardToColors()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString text = clipboard->text();
    text.remove('\n'); // support copy/paste from image-frames
    QStringList parts = text.split(';');
    QList<QPair<int, QColor>> result;
    for (QString part : parts) {
        int hashIdx = part.indexOf('#');
        if (hashIdx < 0) {
            break;
        }
        QColor color = QColor(part.right(7));
        if (!color.isValid()) {
            break;
        }
        part.chop(7);
        bool ok;
        int index = part.toInt(&ok);
        if (!ok) {
            index += result.count();
        }
        result.append(qMakePair(index, color));
    }
    return result;
}

void colorsToClipboard(int startColorIndex, int lastColorIndex, D1Pal *pal)
{
    QString text;
    for (int i = startColorIndex; i <= lastColorIndex; i++) {
        QColor palColor = pal->getColor(i);
        text.append(QString::number(i) + palColor.name() + ';');
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

void PaletteWidget::ShowContextMenu(const QPoint &pos)
{
    this->initStopColorPicking();

    QAction actions[4];

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    int cursor = 0;
    actions[cursor].setText(tr("Undo"));
    actions[cursor].setToolTip(tr("Undo previous color change"));
    actions[cursor].setShortcut(QKeySequence::Undo);
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionUndo_triggered()));
    actions[cursor].setEnabled(this->undoStack->canUndo());
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Redo"));
    actions[cursor].setToolTip(tr("Redo previous color change"));
    actions[cursor].setShortcut(QKeySequence::Redo);
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionRedo_triggered()));
    actions[cursor].setEnabled(this->undoStack->canRedo());
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Copy"));
    actions[cursor].setToolTip(tr("Copy the selected colors to the clipboard"));
    actions[cursor].setShortcut(QKeySequence::Copy);
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionCopy_triggered()));
    actions[cursor].setEnabled(this->selectedFirstColorIndex != COLORIDX_TRANSPARENT);
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Paste"));
    actions[cursor].setToolTip(tr("Paste the colors from the clipboard to the palette"));
    actions[cursor].setShortcut(QKeySequence::Paste);
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionPaste_triggered()));
    actions[cursor].setEnabled(!this->isTrn && !clipboardToColors().isEmpty());
    contextMenu.addAction(&actions[cursor]);

    contextMenu.exec(mapToGlobal(pos));
}

void PaletteWidget::startColorSelection(int colorIndex)
{
    this->prevSelectedColorIndex = (this->selectedFirstColorIndex == this->selectedLastColorIndex) ? this->selectedFirstColorIndex : COLORIDX_TRANSPARENT;
    this->selectedFirstColorIndex = colorIndex;
    this->selectedLastColorIndex = colorIndex;
}

void PaletteWidget::changeColorSelection(int colorIndex)
{
    this->selectedLastColorIndex = colorIndex;

    this->updateFields();
}

void PaletteWidget::changeColorSelection(int dir, bool extend)
{
    if (this->selectedFirstColorIndex == COLORIDX_TRANSPARENT) {
        return;
    }

    switch (dir) {
    case Qt::Key_Left:
        if (this->selectedFirstColorIndex == 0) {
            if (extend) {
                return;
            }
        } else {
            this->selectedFirstColorIndex--;
        }
        break;
    case Qt::Key_Right:
        if (this->selectedLastColorIndex == D1PAL_COLORS - 1) {
            if (extend) {
                return;
            }
        } else {
            this->selectedLastColorIndex++;
        }
        break;
    case Qt::Key_Up:
        if (this->selectedFirstColorIndex == 0) {
            if (extend) {
                return;
            }
        } else {
            this->selectedFirstColorIndex -= PALETTE_COLORS_PER_LINE;
            if (this->selectedFirstColorIndex < 0) {
                this->selectedFirstColorIndex = 0;
            }
        }
        break;
    case Qt::Key_Down:
        if (this->selectedLastColorIndex == D1PAL_COLORS - 1) {
            if (extend) {
                return;
            }
        } else {
            this->selectedLastColorIndex += PALETTE_COLORS_PER_LINE;
            if (this->selectedLastColorIndex > D1PAL_COLORS - 1) {
                this->selectedLastColorIndex = D1PAL_COLORS - 1;
            }
        }
        break;
    default:
        return;
    }

    if (!extend) {    
        switch (dir) {
        case Qt::Key_Left:
        case Qt::Key_Up:
            this->selectedLastColorIndex = this->selectedFirstColorIndex;
            break;
        case Qt::Key_Down:
        case Qt::Key_Right:
            this->selectedFirstColorIndex = this->selectedLastColorIndex;
            break;
            break;
        default:
            return;
        }
    }
    this->updateFields();
}

void PaletteWidget::finishColorSelection()
{
    if (this->selectedFirstColorIndex == this->selectedLastColorIndex) {
        // If only one color is selected which is the same as before -> deselect the colors
        if (this->prevSelectedColorIndex == this->selectedFirstColorIndex) {
            this->selectedFirstColorIndex = COLORIDX_TRANSPARENT;
            this->selectedLastColorIndex = COLORIDX_TRANSPARENT;
        }
    } else if (this->selectedFirstColorIndex > this->selectedLastColorIndex) {
        // If second selected color has an index less than the first one swap them
        std::swap(this->selectedFirstColorIndex, this->selectedLastColorIndex);
    }

    this->updateFields();

    // emit selected colors
    std::vector<quint8> indexes;
    for (int i = this->selectedFirstColorIndex; i <= this->selectedLastColorIndex && i != COLORIDX_TRANSPARENT; i++)
        indexes.push_back(i);

    emit this->colorsSelected(indexes);

    if (this->pickingTranslationColor) {
        emit this->colorsPicked(indexes);
    } else {
        this->initStopColorPicking();
    }
}

void PaletteWidget::initStopColorPicking()
{
    // this->stopTrnColorPicking();

    emit this->colorPicking_stopped(); // cancel color picking
}

void PaletteWidget::displayColors()
{
    // Removing existing items
    this->scene.clear();

    // Displaying palette colors
    D1Pal *colorPal = this->isTrn ? this->trn->getResultingPalette() : this->pal;
    const QPen noPen(Qt::NoPen);
    for (int i = 0; i < D1PAL_COLORS; i++) {
        QColor color = colorPal->getColor(i);

        // Check palette display filter

        // if user just click "Pick" button to select color in parent palette or translation, display all colors
        int indexHits = 1;
        if (!this->pickingTranslationColor) {
            int itemIndex = -1;
            switch (this->palHits->getMode()) {
            case D1PALHITS_MODE::ALL_COLORS:
            case D1PALHITS_MODE::ALL_FRAMES:
                break;
            case D1PALHITS_MODE::CURRENT_TILE:
                itemIndex = this->levelCelView->getCurrentTileIndex();
                break;
            case D1PALHITS_MODE::CURRENT_SUBTILE:
                itemIndex = this->levelCelView->getCurrentSubtileIndex();
                break;
            case D1PALHITS_MODE::CURRENT_FRAME:
                if (this->levelCelView != nullptr) {
                    itemIndex = this->levelCelView->getCurrentFrameIndex();
                } else if (this->celView != nullptr) {
                    itemIndex = this->celView->getCurrentFrameIndex();
                } else if (this->gfxsetView != nullptr) {
                    itemIndex = this->gfxsetView->getCurrentFrameIndex();
                }
                break;
            }
            indexHits = this->palHits->getIndexHits(i, itemIndex);
        }

        bool displayColor = indexHits > 0;

        // Check translation display filter
        if (this->isTrn && ui->displayComboBox->currentData().value<COLORFILTER_TYPE>() == COLORFILTER_TYPE::TRANSLATED // "Show the altered colors"
            && this->trn->getTranslation(i) == i)
            displayColor = false;

        if (displayColor) {
            QRectF coordinates = PaletteScene::getColorCoordinates(i);
            int a = PALETTE_COLOR_SPACING;
            coordinates.adjust(a, a, -a, -a);

            QBrush brush = QBrush(color);

            this->scene.addRect(coordinates, noPen, brush);
        }
    }

    this->displaySelection();
}

void PaletteWidget::displaySelection()
{
    int firstColorIndex = this->selectedFirstColorIndex;
    int lastColorIndex = this->selectedLastColorIndex;
    if (firstColorIndex > lastColorIndex) {
        std::swap(firstColorIndex, lastColorIndex);
    }
    if (firstColorIndex == COLORIDX_TRANSPARENT) {
        return;
    }
    QColor borderColor = QColor(Config::getPaletteSelectionBorderColor());
    QPen pen(borderColor);
    // pen.setStyle(Qt::SolidLine);
    // pen.setJoinStyle(Qt::MiterJoin);
    pen.setWidth(PALETTE_SELECTION_WIDTH);

    for (int i = firstColorIndex; i <= lastColorIndex; i++) {
        QRectF coordinates = PaletteScene::getColorCoordinates(i);
        int a = PALETTE_SELECTION_WIDTH / 2;
        coordinates.adjust(a, a, -a, -a);

        // left line
        if (i == firstColorIndex && i + PALETTE_COLORS_PER_LINE <= lastColorIndex)
            this->scene.addLine(coordinates.bottomLeft().x(), coordinates.bottomLeft().y() + PALETTE_SELECTION_WIDTH,
                coordinates.topLeft().x(), coordinates.topLeft().y(), pen);
        else if (i == firstColorIndex || i % PALETTE_COLORS_PER_LINE == 0)
            this->scene.addLine(coordinates.bottomLeft().x(), coordinates.bottomLeft().y(),
                coordinates.topLeft().x(), coordinates.topLeft().y(), pen);

        // right line
        if (i == lastColorIndex && i - PALETTE_COLORS_PER_LINE >= firstColorIndex)
            this->scene.addLine(coordinates.topRight().x(), coordinates.topRight().y() - PALETTE_SELECTION_WIDTH,
                coordinates.bottomRight().x(), coordinates.bottomRight().y(), pen);
        else if (i == lastColorIndex || i % PALETTE_COLORS_PER_LINE == PALETTE_COLORS_PER_LINE - 1)
            this->scene.addLine(coordinates.topRight().x(), coordinates.topRight().y(),
                coordinates.bottomRight().x(), coordinates.bottomRight().y(), pen);

        // top line
        if (i - PALETTE_COLORS_PER_LINE < firstColorIndex)
            this->scene.addLine(coordinates.topLeft().x(), coordinates.topLeft().y(),
                coordinates.topRight().x(), coordinates.topRight().y(), pen);

        // bottom line
        if (i + PALETTE_COLORS_PER_LINE > lastColorIndex)
            this->scene.addLine(coordinates.bottomLeft().x(), coordinates.bottomLeft().y(),
                coordinates.bottomRight().x(), coordinates.bottomRight().y(), pen);
    }
}

void PaletteWidget::startTrnColorPicking(bool single)
{
    // stop previous picking
    // this->initStopColorPicking();
    // this->stopTrnColorPicking();

    this->ui->graphicsView->setStyleSheet("color: rgb(255, 0, 0);");
    this->ui->informationLabel->setText(tr("<- Select color(s)", "", single ? 1 : 2));
    this->pickingTranslationColor = true;
    this->displayColors();
    // QMessageBox::critical(this, tr("Error"), tr("focus"));
    // this->setFocus();
    this->scene.setFocus();
}

void PaletteWidget::stopTrnColorPicking()
{
    this->ui->graphicsView->setStyleSheet("color: rgb(255, 255, 255);");
    this->ui->informationLabel->clear();
    this->pickingTranslationColor = false;
    this->displayColors();
}

void PaletteWidget::updatePathComboBoxOptions(const QList<QString> &options, const QString &selectedOption)
{
    QComboBox *pcb = this->ui->pathComboBox;

    pcb->clear();
    int idx = 0;
    // add built-in options
    for (const QString &option : options) {
        if (!MainWindow::isResourcePath(option))
            continue;
        QString name = option == D1Pal::DEFAULT_PATH ? tr("_default.pal") : tr("_null.trn"); // TODO: check if D1Trn::IDENTITY_PATH?
        pcb->addItem(name, option);
        if (selectedOption == option) {
            pcb->setCurrentIndex(idx);
            pcb->setToolTip(option);
        }
        idx++;
    }
    // add user-specific options
    for (const QString &option : options) {
        if (MainWindow::isResourcePath(option))
            continue;
        QFileInfo fileInfo(option);
        QString name = fileInfo.fileName();
        pcb->addItem(name, option);
        if (selectedOption == option) {
            pcb->setCurrentIndex(idx);
            pcb->setToolTip(option);
        }
        idx++;
    }
}

void PaletteWidget::refreshColorLineEdit()
{
    QString text;
    int firstColorIndex = this->selectedFirstColorIndex;
    int lastColorIndex = this->selectedLastColorIndex;
    bool active = firstColorIndex != COLORIDX_TRANSPARENT;
    if (active) {
        if (firstColorIndex != lastColorIndex) {
            text = "*";
        } else {
            D1Pal *colorPal = this->isTrn ? this->trn->getResultingPalette() : this->pal;
            text = colorPal->getColor(firstColorIndex).name();
        }
    }
    this->ui->colorLineEdit->setText(text);
    this->ui->colorLineEdit->setReadOnly(this->isTrn || !active);
    this->ui->colorPickPushButton->setEnabled(active);
    this->ui->colorClearPushButton->setEnabled(active);
}

void PaletteWidget::refreshIndexLineEdit()
{
    QString text;
    int firstColorIndex = this->selectedFirstColorIndex;
    int lastColorIndex = this->selectedLastColorIndex;
    bool active = firstColorIndex != COLORIDX_TRANSPARENT;
    if (active) {
        if (firstColorIndex != lastColorIndex) {
            // If second selected color has an index less than the first one swap them
            if (firstColorIndex > lastColorIndex) {
                std::swap(firstColorIndex, lastColorIndex);
            }
            const char *sep = (firstColorIndex < 100 && lastColorIndex < 100) ? " - " : "-";
            text = QString::number(firstColorIndex) + sep + QString::number(lastColorIndex);
        } else {
            text = QString::number(firstColorIndex);
        }
    }
    this->ui->indexLineEdit->setText(text);
}

void PaletteWidget::refreshTranslationIndexLineEdit()
{
    QString text;
    int firstColorIndex = this->selectedFirstColorIndex;
    int lastColorIndex = this->selectedLastColorIndex;
    bool active = firstColorIndex != COLORIDX_TRANSPARENT;
    if (active && this->isTrn) {
        // If second selected color has an index less than the first one swap them
        if (firstColorIndex > lastColorIndex) {
            std::swap(firstColorIndex, lastColorIndex);
        }
        bool match = true;
        quint8 firstTrn = this->trn->getTranslation(firstColorIndex);
        quint8 lastTrn = this->trn->getTranslation(lastColorIndex);
        int dc = firstTrn == lastTrn ? 0 : (firstTrn < lastTrn ? 1 : -1);
        quint8 currTrn = firstTrn;
        for (int i = firstColorIndex; i <= lastColorIndex; i++, currTrn += dc) {
            if (currTrn != this->trn->getTranslation(i)) {
                match = false;
                break;
            }
        }
        if (match) {
            if (firstTrn == lastTrn) {
                text = QString::number(firstTrn);
            } else {
                const char *sep = (firstTrn < 100 && lastTrn < 100) ? " - " : "-";
                text = QString::number(firstTrn) + sep + QString::number(lastTrn);
            }
        } else {
            text = "*";
        }
    }
    this->ui->translationIndexLineEdit->setText(text);
    this->ui->translationIndexLineEdit->setReadOnly(!active);
}

void PaletteWidget::modify()
{
    this->refresh();
    dMainWindow().colorModified();
}

void PaletteWidget::updateFields()
{
    /*if (!this->isVisible() && !refreshing) {
        return;
    }*/
    this->displayColors();
    // this->refreshPathComboBox();
    this->refreshColorLineEdit();
    this->refreshIndexLineEdit();
    this->refreshTranslationIndexLineEdit();
}

void PaletteWidget::refresh()
{
    if (this->isTrn)
        this->trn->refreshResultingPalette();

    this->updateFields();

    emit refreshed();
}

void PaletteWidget::on_newPushButtonClicked()
{
    this->initStopColorPicking();

    dMainWindow().paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_NEW);
}

void PaletteWidget::on_openPushButtonClicked()
{
    this->initStopColorPicking();

    dMainWindow().paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_OPEN);
}

void PaletteWidget::on_savePushButtonClicked()
{
    this->initStopColorPicking();

    dMainWindow().paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVE);
}

void PaletteWidget::on_saveAsPushButtonClicked()
{
    this->initStopColorPicking();

    dMainWindow().paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVEAS);
}

void PaletteWidget::on_closePushButtonClicked()
{
    this->initStopColorPicking();

    dMainWindow().paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_CLOSE);
}

void PaletteWidget::on_actionUndo_triggered()
{
    this->undoStack->undo();
}

void PaletteWidget::on_actionRedo_triggered()
{
    this->undoStack->redo();
}

void PaletteWidget::on_actionCopy_triggered()
{
    D1Pal *palette = this->pal;

    if (this->isTrn) {
        palette = this->trn->getResultingPalette();
    }
    colorsToClipboard(this->selectedFirstColorIndex, this->selectedLastColorIndex, palette);
}

void PaletteWidget::on_actionPaste_triggered()
{
    // assert(!this->isTrn);
    // collect the colors
    QList<QPair<int, QColor>> colors = clipboardToColors();
    if (colors.isEmpty()) {
        return;
    }
    // shift the indices
    int startColorIndex = this->selectedFirstColorIndex;
    if (startColorIndex == COLORIDX_TRANSPARENT) {
        startColorIndex = 0;
    }
    int srcColorIndex = colors[0].first;
    QMap<int, QColor> colorMap;
    for (QPair<int, QColor> &idxColor : colors) {
        int dstColorIndex = startColorIndex + idxColor.first - srcColorIndex;
        if (dstColorIndex < 0 || dstColorIndex > D1PAL_COLORS - 1) {
            continue;
        }
        colorMap[dstColorIndex] = idxColor.second;
    }

    if (colorMap.isEmpty()) {
        return;
    }
    // create list from the map by filling the gaps with the original colors
    startColorIndex = colorMap.firstKey();
    int lastColorIndex = colorMap.lastKey();
    QList<QColor> modColors;
    for (int i = startColorIndex; i <= lastColorIndex; i++) {
        auto iter = colorMap.find(i);
        QColor color;
        if (iter != colorMap.end()) {
            color = iter.value();
        } else {
            color = this->pal->getColor(i);
        }
        modColors.append(color);
    }

    // Build color editing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    EditPaletteCommand *command = new EditPaletteCommand(
        this->pal, startColorIndex, lastColorIndex, modColors);
    QObject::connect(command, &EditPaletteCommand::modified, this, &PaletteWidget::modify);

    this->undoStack->push(command);
}

void PaletteWidget::on_pathComboBox_activated(int index)
{
    this->initStopColorPicking();

    QString path = this->ui->pathComboBox->itemData(index).value<QString>();

    emit this->pathSelected(path);
    // this->modify();
}

void PaletteWidget::on_displayComboBox_activated(int index)
{
    this->initStopColorPicking();

    if (!this->isTrn) {
        D1PALHITS_MODE mode = D1PALHITS_MODE::ALL_COLORS;
        switch (this->ui->displayComboBox->itemData(index).value<COLORFILTER_TYPE>()) {
        case COLORFILTER_TYPE::NONE:
            mode = D1PALHITS_MODE::ALL_COLORS;
            break;
        case COLORFILTER_TYPE::USED:
            mode = D1PALHITS_MODE::ALL_FRAMES;
            break;
        case COLORFILTER_TYPE::TILE:
            mode = D1PALHITS_MODE::CURRENT_TILE;
            break;
        case COLORFILTER_TYPE::SUBTILE:
            mode = D1PALHITS_MODE::CURRENT_SUBTILE;
            break;
        case COLORFILTER_TYPE::FRAME:
            mode = D1PALHITS_MODE::CURRENT_FRAME;
            break;
        }
        this->palHits->setMode(mode);
        // update the TRN widgets
        emit refreshed();
    }

    this->displayColors();
}

void PaletteWidget::on_colorLineEdit_returnPressed()
{
    // assert(!this->isTrn);
    QColor color = QColor(ui->colorLineEdit->text());

    if (color.isValid()) {
        // Build color editing command and connect it to the current palette widget
        // to update the PAL/TRN and CEL views when undo/redo is performed
        EditPaletteCommand *command = new EditPaletteCommand(
            this->pal, this->selectedFirstColorIndex, this->selectedLastColorIndex, color, color);
        QObject::connect(command, &EditPaletteCommand::modified, this, &PaletteWidget::modify);

        this->undoStack->push(command);
    }
    // Release focus to allow keyboard shortcuts to work as expected
    this->on_colorLineEdit_escPressed();
}

void PaletteWidget::on_colorLineEdit_escPressed()
{
    this->initStopColorPicking();

    this->refreshColorLineEdit();
    this->ui->colorLineEdit->clearFocus();
}

void PaletteWidget::on_colorPickPushButton_clicked()
{
    if (this->isTrn) {
        this->ui->colorPickPushButton->clearFocus();
        emit this->colorPicking_started(this->selectedFirstColorIndex == this->selectedLastColorIndex);
    } else {
        this->initStopColorPicking();

        QColor color = QColorDialog::getColor();
        QColor colorEnd;
        if (this->selectedFirstColorIndex == this->selectedLastColorIndex) {
            colorEnd = color;
        } else {
            colorEnd = QColorDialog::getColor();
        }
        if (!color.isValid() || !colorEnd.isValid()) {
            return;
        }
        // Build color editing command and connect it to the current palette widget
        // to update the PAL/TRN and CEL views when undo/redo is performed
        EditPaletteCommand *command = new EditPaletteCommand(
            this->pal, this->selectedFirstColorIndex, this->selectedLastColorIndex, color, colorEnd);
        QObject::connect(command, &EditPaletteCommand::modified, this, &PaletteWidget::modify);

        this->undoStack->push(command);
    }
}

void PaletteWidget::on_colorClearPushButton_clicked()
{
    this->initStopColorPicking();
    if (this->isTrn) {
        // Build translation clearing command and connect it to the current palette widget
        // to update the PAL/TRN and CEL views when undo/redo is performed
        EditTranslationCommand *command = new EditTranslationCommand(
            this->trn, this->selectedFirstColorIndex, this->selectedLastColorIndex, nullptr);
        QObject::connect(command, &EditTranslationCommand::modified, this, &PaletteWidget::modify);

        this->undoStack->push(command);
    } else {
        QColor undefinedColor = QColor(Config::getPaletteUndefinedColor());

        // Build color editing command and connect it to the current palette widget
        // to update the PAL/TRN and CEL views when undo/redo is performed
        EditPaletteCommand *command = new EditPaletteCommand(
            this->pal, this->selectedFirstColorIndex, this->selectedLastColorIndex, undefinedColor, undefinedColor);
        QObject::connect(command, &EditPaletteCommand::modified, this, &PaletteWidget::modify);

        this->undoStack->push(command);
    }
}

void PaletteWidget::on_translationIndexLineEdit_returnPressed()
{
    // assert(!this->isTrn);

    std::pair<int, int> targetRange = ui->translationIndexLineEdit->nonNegRange();
    if (targetRange.first > D1PAL_COLORS || targetRange.second > D1PAL_COLORS) {
        QMessageBox::warning(this, tr("Warning"), tr("Invalid palette index-range."));
        return;
    }
    int firstColorIndex = this->selectedFirstColorIndex;
    int lastColorIndex = this->selectedLastColorIndex;
    // If second selected color has an index less than the first one swap them
    if (firstColorIndex > lastColorIndex) {
        std::swap(firstColorIndex, lastColorIndex);
    }
    if (targetRange.first != targetRange.second && abs(targetRange.second - targetRange.first) != lastColorIndex - firstColorIndex) {
        QMessageBox::warning(this, tr("Warning"), tr("Source and target selection length do not match."));
        return;
    }

    // New translations
    static_assert(D1PAL_COLORS <= std::numeric_limits<quint8>::max() + 1, "on_translationIndexLineEdit_returnPressed stores color indices in quint8.");
    std::vector<quint8> newTranslations;
    int index = targetRange.first;
    const int dc = targetRange.first == targetRange.second ? 0 : (targetRange.first < targetRange.second ? 1 : -1);
    for (int i = firstColorIndex; i <= lastColorIndex; i++, index += dc) {
        if (index == D1PAL_COLORS) {
            newTranslations.push_back(i);
        } else {
            newTranslations.push_back(index);
        }
    }
    // Build translation editing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    EditTranslationCommand *command = new EditTranslationCommand(
        this->trn, firstColorIndex, lastColorIndex, &newTranslations);
    QObject::connect(command, &EditTranslationCommand::modified, this, &PaletteWidget::modify);
    this->undoStack->push(command);

    // Release focus to allow keyboard shortcuts to work as expected
    this->on_translationIndexLineEdit_escPressed();
}

void PaletteWidget::on_translationIndexLineEdit_escPressed()
{
    this->initStopColorPicking();

    this->refreshTranslationIndexLineEdit();
    this->ui->translationIndexLineEdit->clearFocus();
}

void PaletteWidget::patchTrn()
{
    this->initStopColorPicking();

    std::vector<std::pair<quint8, quint8>> modTranslations;
    for (int i = 0; i < D1PAL_COLORS; i++) {
        if (this->trn->getTranslation(i) == 0xFF) {
            modTranslations.push_back(std::pair<quint8, quint8>(i, 0));
        }
    }
    if (!modTranslations.empty()) {
        EditTranslationCommand *command = new EditTranslationCommand(this->trn, modTranslations);
        QObject::connect(command, &EditTranslationCommand::modified, this, &PaletteWidget::modify);

        this->undoStack->push(command);
    }
}

void PaletteWidget::keyPressEvent(QKeyEvent *event)
{
    if (this->pickingTranslationColor && event->matches(QKeySequence::Cancel)) {
        this->initStopColorPicking();
        return;
    }
    if (event->matches(QKeySequence::Copy)) {
        if (this->selectedFirstColorIndex != COLORIDX_TRANSPARENT) {
            this->on_actionCopy_triggered();
        }
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        this->on_actionPaste_triggered();
        return;
    }

    QWidget::keyPressEvent(event);
}
