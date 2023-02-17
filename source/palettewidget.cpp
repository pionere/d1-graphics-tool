#include "palettewidget.h"

#include <algorithm>

#include <QClipboard>
#include <QColorDialog>
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

        this->modColors.append(color);
    }
}

EditPaletteCommand::EditPaletteCommand(D1Pal *p, quint8 sci, quint8 eci, QList<QColor> mc)
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
        this->pal->setColor(i, this->modColors.at(i - startColorIndex));
        this->modColors.replace(i - startColorIndex, palColor);
    }

    emit this->modified();
}

void EditPaletteCommand::redo()
{
    this->undo();
}

EditTranslationCommand::EditTranslationCommand(D1Trn *t, quint8 sci, quint8 eci, QList<quint8> *nt)
    : QUndoCommand(nullptr)
    , trn(t)
    , startColorIndex(sci)
    , endColorIndex(eci)
{
    if (nt == nullptr) {
        for (int i = startColorIndex; i <= endColorIndex; i++)
            modTranslations.append(i);
    } else {
        this->modTranslations = *nt;
    }
}

void EditTranslationCommand::undo()
{
    if (this->trn.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (int i = startColorIndex; i <= endColorIndex; i++) {
        quint8 trnValue = this->trn->getTranslation(i);
        this->trn->setTranslation(i, this->modTranslations.at(i - startColorIndex));
        this->modTranslations.replace(i - startColorIndex, trnValue);
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
}

static int getColorIndexFromCoordinates(QPointF coordinates)
{
    // if (position.x() < 0 || position.x() >= PALETTE_WIDTH
    //    || position.y() < 0 || position.y() >= PALETTE_WIDTH)
    //    return -1;

    int index = 0;

    int w = PALETTE_WIDTH / PALETTE_COLORS_PER_LINE;

    int ix = coordinates.x() / w;
    int iy = coordinates.y() / w;

    index = iy * PALETTE_COLORS_PER_LINE + ix;

    return index;
}

void PaletteScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    QPointF pos = event->scenePos();

    qDebug() << QStringLiteral("Clicked: %1:%2").arg(pos.x()).arg(pos.y());

    // Check if selected color has changed
    int colorIndex = getColorIndexFromCoordinates(pos);

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
    int colorIndex = getColorIndexFromCoordinates(pos);

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
    , scene(new PaletteScene(this))
{
    ui->setupUi(this);
    ui->graphicsView->setScene(this->scene);
    ui->groupLabel->setText(title);

    QLayout *layout = this->ui->horizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("New"), this, &PaletteWidget::on_newPushButtonClicked); // use SP_FileIcon ?
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogOpenButton, tr("Open"), this, &PaletteWidget::on_openPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogSaveButton, tr("Save"), this, &PaletteWidget::on_savePushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogSaveButton, tr("Save As"), this, &PaletteWidget::on_saveAsPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &PaletteWidget::on_closePushButtonClicked); // use SP_DialogDiscardButton ?

    // When there is a modification to the PAL or TRNs then UI must be refreshed
    QObject::connect(this, &PaletteWidget::modified, this, &PaletteWidget::refresh);

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
    delete scene;
}

void PaletteWidget::setPal(D1Pal *p)
{
    this->pal = p;

    // this->refreshPathComboBox();

    emit this->modified();
}

void PaletteWidget::setTrn(D1Trn *t)
{
    this->trn = t;

    // this->refreshPathComboBox();

    emit this->modified();
}

bool PaletteWidget::isTrnWidget()
{
    return this->isTrn;
}

void PaletteWidget::initialize(D1Pal *p, CelView *c, LevelCelView *lc, D1PalHits *ph)
{
    this->isTrn = false;
    this->pal = p;
    this->trn = nullptr;
    this->celView = c;
    this->levelCelView = lc;
    this->palHits = ph;

    this->initializeUi();
}

void PaletteWidget::initialize(D1Trn *t, CelView *c, LevelCelView *lc, D1PalHits *ph)
{
    this->isTrn = true;
    this->pal = nullptr;
    this->trn = t;
    this->celView = c;
    this->levelCelView = lc;
    this->palHits = ph;

    this->initializeUi();
}

void PaletteWidget::initializeUi()
{
    bool trnMode = this->isTrn;

    this->ui->monsterTrnPushButton->setVisible(trnMode);
    this->ui->translationClearPushButton->setVisible(trnMode);
    this->ui->translationPickPushButton->setVisible(trnMode);
    this->ui->colorPickPushButton->setVisible(!trnMode);
    this->ui->colorClearPushButton->setVisible(!trnMode);

    this->ui->translationIndexLineEdit->setToolTip(trnMode ? tr("Enter the color index to which the selected color(s) should map to.") : tr("Enter the color index or 256 to replace the selected color(s) of the frame(s) with the given color or transparent pixel."));

    // this->initializePathComboBox();
    this->initializeDisplayComboBox();

    this->refreshColorLineEdit();
    this->refreshIndexLineEdit();
    if (trnMode)
        this->refreshTranslationIndexLineEdit();

    this->displayColors();
}

void PaletteWidget::initializeDisplayComboBox()
{
    ui->displayComboBox->addItem(tr("Show all colors"), QVariant((int)COLORFILTER_TYPE::NONE));

    if (!this->isTrn) {
        ui->displayComboBox->addItem(tr("Show all frames hits"), QVariant((int)COLORFILTER_TYPE::USED));
        if (this->levelCelView != nullptr) {
            ui->displayComboBox->addItem(tr("Show current tile hits"), QVariant((int)COLORFILTER_TYPE::TILE));
            ui->displayComboBox->addItem(tr("Show current subtile hits"), QVariant((int)COLORFILTER_TYPE::SUBTILE));
        }
        ui->displayComboBox->addItem(tr("Show current frame hits"), QVariant((int)COLORFILTER_TYPE::FRAME));
    } else {
        ui->displayComboBox->addItem(tr("Show the altered colors"), QVariant((int)COLORFILTER_TYPE::TRANSLATED));
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

    this->refresh();
}

D1GfxPixel PaletteWidget::getCurrentColor(unsigned counter) const
{
    if (this->selectedFirstColorIndex == COLORIDX_TRANSPARENT) {
        return D1GfxPixel::transparentPixel();
    }

    unsigned numColors = this->selectedLastColorIndex - this->selectedFirstColorIndex + 1;
    return D1GfxPixel::colorPixel(this->selectedFirstColorIndex + (counter % numColors));
}

void PaletteWidget::checkTranslationsSelection(QList<quint8> indexes)
{
    int selectionLength = this->selectedLastColorIndex - this->selectedFirstColorIndex + 1;
    if (selectionLength != indexes.length()) {
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

static QRectF getColorCoordinates(quint8 index)
{
    int ix = index % PALETTE_COLORS_PER_LINE;
    int iy = index / PALETTE_COLORS_PER_LINE;

    int w = PALETTE_WIDTH / PALETTE_COLORS_PER_LINE;

    QRectF coordinates(ix * w, iy * w, w, w);

    return coordinates;
}

QList<QPair<int, QColor>> clipboardToColors()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString text = clipboard->text();
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

    this->refreshIndexLineEdit();

    this->displayColors();
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

    this->refresh();

    if (this->pickingTranslationColor) {
        if (this->selectedFirstColorIndex == COLORIDX_TRANSPARENT) {
            return; // empty selection -> skip
        }
        // emit selected colors
        QList<quint8> indexes;
        for (int i = this->selectedFirstColorIndex; i <= this->selectedLastColorIndex; i++)
            indexes.append(i);

        emit this->colorsSelected(indexes);
    } else {
        this->initStopColorPicking();
    }
}

void PaletteWidget::initStopColorPicking()
{
    this->stopTrnColorPicking();

    emit this->colorPicking_stopped(); // cancel color picking
}

void PaletteWidget::displayColors()
{
    // Positions
    int x = 0;
    int y = 0;

    // X delta
    int dx = PALETTE_WIDTH / PALETTE_COLORS_PER_LINE;
    // Y delta
    int dy = PALETTE_WIDTH / PALETTE_COLORS_PER_LINE;

    // Color width
    int w = PALETTE_WIDTH / PALETTE_COLORS_PER_LINE - 2 * PALETTE_COLOR_SPACING;
    int bsw = PALETTE_COLOR_SPACING;

    // Removing existing items
    this->scene->clear();

    // Setting background color
    this->scene->setBackgroundBrush(Qt::white);

    // Displaying palette colors
    D1Pal *colorPal = this->isTrn ? this->trn->getResultingPalette() : this->pal;
    for (int i = 0; i < D1PAL_COLORS; i++) {
        // Go to next line
        if (i % PALETTE_COLORS_PER_LINE == 0 && i != 0) {
            x = 0;
            y += dy;
        }

        QColor color = colorPal->getColor(i);
        QBrush brush = QBrush(color);
        QPen pen(Qt::NoPen);

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
                } else {
                    itemIndex = this->celView->getCurrentFrameIndex();
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

        if (displayColor)
            this->scene->addRect(x + bsw, y + bsw, w, w, pen, brush);

        x += dx;
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
        QRectF coordinates = getColorCoordinates(i);
        int a = PALETTE_SELECTION_WIDTH / 2;
        coordinates.adjust(a, a, -a, -a);

        // left line
        if (i == firstColorIndex && i + PALETTE_COLORS_PER_LINE <= lastColorIndex)
            this->scene->addLine(coordinates.bottomLeft().x(), coordinates.bottomLeft().y() + PALETTE_SELECTION_WIDTH,
                coordinates.topLeft().x(), coordinates.topLeft().y(), pen);
        else if (i == firstColorIndex || i % PALETTE_COLORS_PER_LINE == 0)
            this->scene->addLine(coordinates.bottomLeft().x(), coordinates.bottomLeft().y(),
                coordinates.topLeft().x(), coordinates.topLeft().y(), pen);

        // right line
        if (i == lastColorIndex && i - PALETTE_COLORS_PER_LINE >= firstColorIndex)
            this->scene->addLine(coordinates.topRight().x(), coordinates.topRight().y() - PALETTE_SELECTION_WIDTH,
                coordinates.bottomRight().x(), coordinates.bottomRight().y(), pen);
        else if (i == lastColorIndex || i % PALETTE_COLORS_PER_LINE == PALETTE_COLORS_PER_LINE - 1)
            this->scene->addLine(coordinates.topRight().x(), coordinates.topRight().y(),
                coordinates.bottomRight().x(), coordinates.bottomRight().y(), pen);

        // top line
        if (i - PALETTE_COLORS_PER_LINE < firstColorIndex)
            this->scene->addLine(coordinates.topLeft().x(), coordinates.topLeft().y(),
                coordinates.topRight().x(), coordinates.topRight().y(), pen);

        // bottom line
        if (i + PALETTE_COLORS_PER_LINE > lastColorIndex)
            this->scene->addLine(coordinates.bottomLeft().x(), coordinates.bottomLeft().y(),
                coordinates.bottomRight().x(), coordinates.bottomRight().y(), pen);
    }
}

void PaletteWidget::startTrnColorPicking(bool single)
{
    // stop previous picking
    this->initStopColorPicking();

    this->ui->graphicsView->setStyleSheet("color: rgb(255, 0, 0);");
    this->ui->informationLabel->setText(tr("<- Select color(s)", "", single ? 1 : 2));
    this->pickingTranslationColor = true;
    this->displayColors();
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
    int colorIndex = this->selectedFirstColorIndex;
    QString text;
    bool active = !this->isTrn;

    if (colorIndex != this->selectedLastColorIndex) {
        text = "*";
    } else if (colorIndex != COLORIDX_TRANSPARENT) {
        D1Pal *colorPal = this->isTrn ? this->trn->getResultingPalette() : this->pal;
        text = colorPal->getColor(colorIndex).name();
    } else {
        active = false;
    }
    this->ui->colorLineEdit->setText(text);
    this->ui->colorLineEdit->setReadOnly(!active);
    this->ui->colorPickPushButton->setEnabled(active);
    this->ui->colorClearPushButton->setEnabled(active);
}

void PaletteWidget::refreshIndexLineEdit()
{
    QString text;

    int firstColorIndex = this->selectedFirstColorIndex;
    int lastColorIndex = this->selectedLastColorIndex;
    if (firstColorIndex != lastColorIndex) {
        // If second selected color has an index less than the first one swap them
        if (firstColorIndex > lastColorIndex) {
            std::swap(firstColorIndex, lastColorIndex);
        }
        const char *sep = (firstColorIndex < 100 && lastColorIndex < 100) ? " - " : "-";
        text = QString::number(firstColorIndex) + sep + QString::number(lastColorIndex);
    } else if (firstColorIndex != COLORIDX_TRANSPARENT) {
        text = QString::number(firstColorIndex);
    }
    this->ui->indexLineEdit->setText(text);
}

void PaletteWidget::refreshTranslationIndexLineEdit()
{
    int colorIndex = this->selectedFirstColorIndex;
    QString text;
    bool active = colorIndex != COLORIDX_TRANSPARENT;

    if (active && this->isTrn) {
        if (colorIndex != this->selectedLastColorIndex) {
            text = "*";
        } else {
            text = QString::number(this->trn->getTranslation(colorIndex));
        }
    }
    this->ui->translationIndexLineEdit->setText(text);
    this->ui->translationIndexLineEdit->setReadOnly(!active);
    this->ui->translationPickPushButton->setEnabled(active);
    this->ui->translationClearPushButton->setEnabled(active);
}

void PaletteWidget::modify()
{
    emit this->modified();
}

void PaletteWidget::refresh()
{
    if (this->isTrn)
        this->trn->refreshResultingPalette();

    this->displayColors();
    // this->refreshPathComboBox();
    this->refreshColorLineEdit();
    this->refreshIndexLineEdit();
    if (this->isTrn)
        this->refreshTranslationIndexLineEdit();

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
    // emit this->modified();
}

void PaletteWidget::on_displayComboBox_activated(int index)
{
    this->initStopColorPicking();

    if (!this->isTrn) {
        D1PALHITS_MODE mode = D1PALHITS_MODE::ALL_COLORS;
        switch (this->ui->displayComboBox->currentData().value<COLORFILTER_TYPE>()) {
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
    }

    this->refresh();
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
    // assert(!this->isTrn);
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

void PaletteWidget::on_colorClearPushButton_clicked()
{
    // assert(!this->isTrn);
    this->initStopColorPicking();

    QColor undefinedColor = QColor(Config::getPaletteUndefinedColor());

    // Build color editing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    auto *command = new EditPaletteCommand(
        this->pal, this->selectedFirstColorIndex, this->selectedLastColorIndex, undefinedColor, undefinedColor);
    QObject::connect(command, &EditPaletteCommand::modified, this, &PaletteWidget::modify);

    this->undoStack->push(command);
}

void PaletteWidget::on_translationIndexLineEdit_returnPressed()
{
    bool ok;
    unsigned index = ui->translationIndexLineEdit->text().toUInt(&ok);

    if (ok) {
        if (this->isTrn) {
            // New translations
            if (index < D1PAL_COLORS) {
                static_assert(D1PAL_COLORS <= std::numeric_limits<quint8>::max() + 1, "on_translationIndexLineEdit_returnPressed stores color indices in quint8.");
                QList<quint8> newTranslations;
                for (int i = this->selectedFirstColorIndex; i <= this->selectedLastColorIndex; i++)
                    newTranslations.append(index);

                // Build translation editing command and connect it to the current palette widget
                // to update the PAL/TRN and CEL views when undo/redo is performed
                EditTranslationCommand *command = new EditTranslationCommand(
                    this->trn, this->selectedFirstColorIndex, this->selectedLastColorIndex, &newTranslations);
                QObject::connect(command, &EditTranslationCommand::modified, this, &PaletteWidget::modify);

                this->undoStack->push(command);
            }
        } else {
            // Color replacement
            bool needsReplacement = index <= D1PAL_COLORS && (this->selectedFirstColorIndex != this->selectedLastColorIndex || this->selectedFirstColorIndex != index);
            if (needsReplacement) {
                QString replacement = index == D1PAL_COLORS ? tr("transparent pixels") : tr("color %1").arg(index);
                QString range = this->ui->indexLineEdit->text();
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Pixels with color %1 are going to be replaced with %2. This change is not reversible. Are you sure you want to proceed?").arg(range).arg(replacement), QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No);
                if (reply != QMessageBox::No) {
                    D1GfxPixel replacement = index == D1PAL_COLORS ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(index);
                    emit this->changeColor(this->selectedFirstColorIndex, this->selectedLastColorIndex, replacement, reply == QMessageBox::YesToAll);
                }
            }
        }
    }
    // Release focus to allow keyboard shortcuts to work as expected
    this->on_translationIndexLineEdit_escPressed();
}

void PaletteWidget::on_translationIndexLineEdit_escPressed()
{
    this->initStopColorPicking();

    this->refreshTranslationIndexLineEdit();
    this->ui->translationIndexLineEdit->clearFocus();
}

void PaletteWidget::on_translationPickPushButton_clicked()
{
    emit this->colorPicking_started(this->selectedFirstColorIndex == this->selectedLastColorIndex);
}

void PaletteWidget::on_translationClearPushButton_clicked()
{
    this->initStopColorPicking();

    // Build translation clearing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    EditTranslationCommand *command = new EditTranslationCommand(
        this->trn, this->selectedFirstColorIndex, this->selectedLastColorIndex, nullptr);
    QObject::connect(command, &EditTranslationCommand::modified, this, &PaletteWidget::modify);

    this->undoStack->push(command);
}

void PaletteWidget::on_monsterTrnPushButton_clicked()
{
    this->initStopColorPicking();

    bool trnModified = false;

    for (int i = 0; i < D1PAL_COLORS; i++) {
        if (this->trn->getTranslation(i) == 0xFF) {
            this->trn->setTranslation(i, 0);
            trnModified = true;
        }
    }

    if (trnModified) {
        emit this->modified();
    }
}

void PaletteWidget::keyPressEvent(QKeyEvent *event)
{
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
