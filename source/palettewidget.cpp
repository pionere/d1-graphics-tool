#include "palettewidget.h"

#include <algorithm>

#include <QClipboard>
#include <QColorDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMimeData>

#include "config.h"
#include "mainwindow.h"
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

EditPaletteCommand::EditPaletteCommand(D1Pal *p, quint8 sci, quint8 eci, QColor nc, QColor ec, QUndoCommand *parent)
    : QUndoCommand(parent)
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

EditPaletteCommand::EditPaletteCommand(D1Pal *p, quint8 sci, quint8 eci, QList<QColor> mc, QUndoCommand *parent)
    : QUndoCommand(parent)
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

EditTranslationCommand::EditTranslationCommand(D1Trn *t, quint8 sci, quint8 eci, QList<quint8> *nt, QUndoCommand *parent)
    : QUndoCommand(parent)
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

PaletteScene::PaletteScene(QWidget *v)
    : QGraphicsScene(0, 0, PALETTE_WIDTH, PALETTE_WIDTH)
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
    ((PaletteWidget *)this->view)->startColorSelection(colorIndex);
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
    ((PaletteWidget *)this->view)->changeColorSelection(colorIndex);
}

void PaletteScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    ((PaletteWidget *)this->view)->finishColorSelection();
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
    ((MainWindow *)this->view->window())->openPalFiles(filePaths, (PaletteWidget *)this->view);
}

void PaletteScene::contextMenuEvent(QContextMenuEvent *event)
{
    ((PaletteWidget *)this->view)->ShowContextMenu(event->globalPos());
}

QPushButton *PaletteWidget::addButton(QStyle::StandardPixmap type, QString tooltip, void (PaletteWidget::*callback)(void))
{
    QPushButton *button = new QPushButton(this->style()->standardIcon(type), "", nullptr);
    constexpr int iconSize = 16;
    button->setToolTip(tooltip);
    button->setIconSize(QSize(iconSize, iconSize));
    button->setMinimumSize(iconSize, iconSize);
    button->setMaximumSize(iconSize, iconSize);
    this->ui->horizontalLayout->addWidget(button); //, Qt::AlignLeft);

    QObject::connect(button, &QPushButton::clicked, this, callback);
    return button;
}

PaletteWidget::PaletteWidget(QUndoStack *us, QString title)
    : QWidget(nullptr)
    , undoStack(us)
    , ui(new Ui::PaletteWidget())
    , scene(new PaletteScene(this))
{
    ui->setupUi(this);
    ui->graphicsView->setScene(this->scene);
    ui->groupLabel->setText(title);

    this->addButton(QStyle::SP_FileDialogNewFolder, tr("New"), &PaletteWidget::on_newPushButtonClicked); // use SP_FileIcon ?
    this->addButton(QStyle::SP_DialogOpenButton, tr("Open"), &PaletteWidget::on_openPushButtonClicked);
    this->addButton(QStyle::SP_DialogSaveButton, tr("Save"), &PaletteWidget::on_savePushButtonClicked);
    this->addButton(QStyle::SP_DialogSaveButton, tr("Save As"), &PaletteWidget::on_saveAsPushButtonClicked);
    this->addButton(QStyle::SP_DialogCloseButton, tr("Close"), &PaletteWidget::on_closePushButtonClicked); // use SP_DialogDiscardButton ?

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

    QString path = p->getFilePath();
    int index = this->ui->pathComboBox->findData(path);
    this->ui->pathComboBox->setCurrentIndex(index);
    this->ui->pathComboBox->setToolTip(path);

    emit this->modified();
}

void PaletteWidget::setTrn(D1Trn *t)
{
    this->trn = t;

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

void PaletteWidget::initialize(D1Pal *p, D1Trn *t, CelView *c, LevelCelView *lc, D1PalHits *ph)
{
    this->isTrn = true;
    this->pal = p;
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

    this->initializePathComboBox();
    this->initializeDisplayComboBox();

    this->refreshColorLineEdit();
    this->refreshIndexLineEdit();
    if (trnMode)
        this->refreshTranslationIndexLineEdit();

    this->displayColors();
}

void PaletteWidget::initializePathComboBox()
{
    if (!this->isTrn) {
        this->paths[D1Pal::DEFAULT_PATH] = D1Pal::DEFAULT_NAME;
    } else {
        this->paths[D1Trn::IDENTITY_PATH] = D1Trn::IDENTITY_NAME;
    }

    this->refreshPathComboBox();
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

void PaletteWidget::addPath(const QString &path, const QString &name)
{
    this->paths[path] = name;
}

void PaletteWidget::removePath(QString path)
{
    if (this->paths.contains(path))
        this->paths.remove(path);
}

QString PaletteWidget::getSelectedPath() const
{
    return this->paths.key(this->ui->pathComboBox->currentText());
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

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    QAction action0(tr("Undo"), this);
    action0.setToolTip(tr("Undo previous color change"));
    QObject::connect(&action0, SIGNAL(triggered()), this, SLOT(on_actionUndo_triggered()));
    action0.setEnabled(this->undoStack->canUndo());
    contextMenu.addAction(&action0);

    QAction action1(tr("Redo"), this);
    action1.setToolTip(tr("Redo previous color change"));
    QObject::connect(&action1, SIGNAL(triggered()), this, SLOT(on_actionRedo_triggered()));
    action1.setEnabled(this->undoStack->canRedo());
    contextMenu.addAction(&action1);

    QAction action2(tr("Copy"), this);
    action2.setToolTip(tr("Copy the selected colors to the clipboard"));
    QObject::connect(&action2, SIGNAL(triggered()), this, SLOT(on_actionCopy_triggered()));
    action2.setEnabled(this->selectedFirstColorIndex != COLORIDX_TRANSPARENT);
    contextMenu.addAction(&action2);

    QAction action3(tr("Paste"), this);
    action3.setToolTip(tr("Paste the colors from the clipboard to the palette"));
    QObject::connect(&action3, SIGNAL(triggered()), this, SLOT(on_actionPaste_triggered()));
    action3.setEnabled(!this->isTrn && !clipboardToColors().isEmpty());
    contextMenu.addAction(&action3);

    contextMenu.exec(mapToGlobal(pos));
}

void PaletteWidget::startColorSelection(int colorIndex)
{
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
    // If second selected color has an index less than the first one swap them
    if (this->selectedFirstColorIndex > this->selectedLastColorIndex) {
        std::swap(this->selectedFirstColorIndex, this->selectedLastColorIndex);
    }

    this->refresh();

    if (this->pickingTranslationColor) {
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
    for (int i = 0; i < D1PAL_COLORS; i++) {
        // Go to next line
        if (i % PALETTE_COLORS_PER_LINE == 0 && i != 0) {
            x = 0;
            y += dy;
        }

        QColor color = this->isTrn ? this->trn->getResultingPalette()->getColor(i) : this->pal->getColor(i);
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

void PaletteWidget::refreshPathComboBox()
{
    this->ui->pathComboBox->clear();

    // Go through the hits of the CEL frame and add them to the subtile hits
    for (auto iter = this->paths.cbegin(); iter != this->paths.cend(); iter++) {
        this->ui->pathComboBox->addItem(iter.value(), iter.key());
    }

    QString selectedPath;
    if (!this->isTrn) {
        selectedPath = this->pal->getFilePath();
    } else {
        selectedPath = this->trn->getFilePath();
    }
    this->ui->pathComboBox->setCurrentIndex(this->ui->pathComboBox->findData(selectedPath));
    this->ui->pathComboBox->setToolTip(selectedPath);
}

void PaletteWidget::refreshColorLineEdit()
{
    int colorIndex = this->selectedFirstColorIndex;
    QString text;
    bool active = !this->isTrn;

    if (colorIndex != this->selectedLastColorIndex) {
        text = "*";
    } else if (colorIndex != COLORIDX_TRANSPARENT) {
        QColor selectedColor = this->pal->getColor(colorIndex);
        text = selectedColor.name();
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
    this->refreshPathComboBox();
    this->refreshColorLineEdit();
    this->refreshIndexLineEdit();
    if (this->isTrn)
        this->refreshTranslationIndexLineEdit();

    emit refreshed();
}

void PaletteWidget::on_newPushButtonClicked()
{
    this->initStopColorPicking();

    ((MainWindow *)this->window())->paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_NEW);
}

void PaletteWidget::on_openPushButtonClicked()
{
    this->initStopColorPicking();

    ((MainWindow *)this->window())->paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_OPEN);
}

void PaletteWidget::on_savePushButtonClicked()
{
    this->initStopColorPicking();

    ((MainWindow *)this->window())->paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVE);
}

void PaletteWidget::on_saveAsPushButtonClicked()
{
    this->initStopColorPicking();

    ((MainWindow *)this->window())->paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVEAS);
}

void PaletteWidget::on_closePushButtonClicked()
{
    this->initStopColorPicking();

    ((MainWindow *)this->window())->paletteWidget_callback(this, PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_CLOSE);
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
