#include "palettewidget.h"

#include <algorithm>

#include <QColorDialog>
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

EditColorsCommand::EditColorsCommand(D1Pal *p, quint8 sci, quint8 eci, QColor nc, QColor ec, QUndoCommand *parent)
    : QUndoCommand(parent)
    , pal(p)
    , startColorIndex(sci)
    , endColorIndex(eci)
    , newColor(nc)
    , endColor(ec)
{
    // Get the initial color values before doing any modification
    for (int i = startColorIndex; i <= endColorIndex; i++)
        initialColors.append(this->pal->getColor(i));
}

void EditColorsCommand::undo()
{
    if (this->pal.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (int i = startColorIndex; i <= endColorIndex; i++)
        this->pal->setColor(i, this->initialColors.at(i - this->startColorIndex));

    emit this->modified();
}

void EditColorsCommand::redo()
{
    if (this->pal.isNull()) {
        this->setObsolete(true);
        return;
    }

    float step = 1.0f / (endColorIndex - startColorIndex + 1);

    for (int i = startColorIndex; i <= endColorIndex; i++) {
        float factor = (i - startColorIndex) * step;

        QColor color(
            this->newColor.red() * (1 - factor) + this->endColor.red() * factor,
            this->newColor.green() * (1 - factor) + this->endColor.green() * factor,
            this->newColor.blue() * (1 - factor) + this->endColor.blue() * factor);

        this->pal->setColor(i, color);
    }

    emit this->modified();
}

EditTranslationsCommand::EditTranslationsCommand(D1Trn *t, quint8 sci, quint8 eci, QList<quint8> nt, QUndoCommand *parent)
    : QUndoCommand(parent)
    , trn(t)
    , startColorIndex(sci)
    , endColorIndex(eci)
    , newTranslations(nt)
{
    // Get the initial color values before doing any modification
    for (int i = startColorIndex; i <= endColorIndex; i++)
        initialTranslations.append(this->trn->getTranslation(i));
}

void EditTranslationsCommand::undo()
{
    if (this->trn.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (int i = startColorIndex; i <= endColorIndex; i++)
        this->trn->setTranslation(i, this->initialTranslations.at(i - this->startColorIndex));

    emit this->modified();
}

void EditTranslationsCommand::redo()
{
    if (this->trn.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (int i = startColorIndex; i <= endColorIndex; i++)
        this->trn->setTranslation(i, this->newTranslations.at(i - this->startColorIndex));

    emit this->modified();
}

ClearTranslationsCommand::ClearTranslationsCommand(D1Trn *t, quint8 sci, quint8 eci, QUndoCommand *parent)
    : QUndoCommand(parent)
    , trn(t)
    , startColorIndex(sci)
    , endColorIndex(eci)
{
    // Get the initial color values before doing any modification
    for (int i = startColorIndex; i <= endColorIndex; i++)
        initialTranslations.append(this->trn->getTranslation(i));
}

void ClearTranslationsCommand::undo()
{
    if (this->trn.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (int i = startColorIndex; i <= endColorIndex; i++)
        this->trn->setTranslation(i, this->initialTranslations.at(i - this->startColorIndex));

    emit this->modified();
}

void ClearTranslationsCommand::redo()
{
    if (this->trn.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (int i = startColorIndex; i <= endColorIndex; i++)
        this->trn->setTranslation(i, i);

    emit this->modified();
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

    qDebug() << "Clicked: " << pos.x() << "," << pos.y();

    // Check if selected color has changed
    int colorIndex = getColorIndexFromCoordinates(pos);

    // if (colorIndex >= 0)
    ((PaletteWidget *)this->view)->startColorSelection(colorIndex);
}

void PaletteScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // if (event->button() != Qt::LeftButton) {
    //    return;
    // }

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
    QPushButton *button = new QPushButton(this->style()->standardIcon(type), tr(""), nullptr);
    constexpr int iconSize = 16;
    button->setToolTip(tooltip);
    button->setIconSize(QSize(iconSize, iconSize));
    button->setMinimumSize(iconSize, iconSize);
    button->setMaximumSize(iconSize, iconSize);
    ((QBoxLayout *)ui->groupHeader->layout())->addWidget(button, Qt::AlignLeft);

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

    this->addButton(QStyle::SP_FileDialogNewFolder, "New", &PaletteWidget::on_newPushButtonClicked); // use SP_FileIcon ?
    this->addButton(QStyle::SP_DialogOpenButton, "Open", &PaletteWidget::on_openPushButtonClicked);
    this->addButton(QStyle::SP_DialogSaveButton, "Save", &PaletteWidget::on_savePushButtonClicked);
    this->addButton(QStyle::SP_DialogSaveButton, "Save As", &PaletteWidget::on_saveAsPushButtonClicked);
    this->addButton(QStyle::SP_DialogCloseButton, "Close", &PaletteWidget::on_closePushButtonClicked); // use SP_DialogDiscardButton ?

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
    this->ui->translationIndexLineEdit->setVisible(trnMode);
    this->ui->translationLabel->setVisible(trnMode);

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
    ui->displayComboBox->addItem("Show all colors", QVariant((int)COLORFILTER_TYPE::NONE));

    if (!this->isTrn) {
        ui->displayComboBox->addItem("Show all frames hits", QVariant((int)COLORFILTER_TYPE::USED));
        if (this->levelCelView != nullptr) {
            ui->displayComboBox->addItem("Show current tile hits", QVariant((int)COLORFILTER_TYPE::TILE));
            ui->displayComboBox->addItem("Show current sub-tile hits", QVariant((int)COLORFILTER_TYPE::SUBTILE));
        }
        ui->displayComboBox->addItem("Show current frame hits", QVariant((int)COLORFILTER_TYPE::FRAME));
    } else {
        ui->displayComboBox->addItem("Show translated colors", QVariant((int)COLORFILTER_TYPE::TRANSLATED));
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

void PaletteWidget::checkTranslationsSelection(QList<quint8> indexes)
{
    int selectionLength = this->selectedLastColorIndex - this->selectedFirstColorIndex + 1;
    if (selectionLength != indexes.length()) {
        QMessageBox::warning(this, "Warning", "Source and target selection length do not match.");
        return;
    }

    // Build color editing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    EditTranslationsCommand *command = new EditTranslationsCommand(
        this->trn, this->selectedFirstColorIndex, this->selectedLastColorIndex, indexes);
    QObject::connect(command, &EditTranslationsCommand::modified, this, &PaletteWidget::modify);

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

void PaletteWidget::selectPath(QString path)
{
    this->ui->pathComboBox->setCurrentIndex(this->ui->pathComboBox->findData(path));
    this->ui->pathComboBox->setToolTip(path);

    emit this->pathSelected(path);
    emit this->modified();
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

void PaletteWidget::ShowContextMenu(const QPoint &pos)
{
    this->initStopColorPicking();

    QMenu contextMenu(tr("Context menu"), this);
    contextMenu.setToolTipsVisible(true);

    QAction action0("Undo", this);
    QObject::connect(&action0, SIGNAL(triggered()), this, SLOT(on_actionUndo_triggered()));
    action0.setEnabled(this->undoStack->canUndo());
    contextMenu.addAction(&action0);

    QAction action1("Redo", this);
    QObject::connect(&action1, SIGNAL(triggered()), this, SLOT(on_actionRedo_triggered()));
    action1.setEnabled(this->undoStack->canRedo());
    contextMenu.addAction(&action1);

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
        if (this->isTrn && ui->displayComboBox->currentData().value<COLORFILTER_TYPE>() == COLORFILTER_TYPE::TRANSLATED // "Show translated colors"
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

void PaletteWidget::startTrnColorPicking()
{
    // stop previous picking
    this->initStopColorPicking();

    this->ui->graphicsView->setStyleSheet("color: rgb(255, 0, 0);");
    this->ui->informationLabel->setText("<- Select translation");
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
    bool active = true;

    if (colorIndex != this->selectedLastColorIndex) {
        text = "*";
    } else if (colorIndex != COLORIDX_TRANSPARENT) {
        text = QString::number(this->trn->getTranslation(colorIndex));
    } else {
        active = false;
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

void PaletteWidget::on_pathComboBox_activated(int index)
{
    this->initStopColorPicking();

    QString filePath = this->ui->pathComboBox->currentData().value<QString>();

    emit this->pathSelected(filePath);
    emit this->modified();
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
        EditColorsCommand *command = new EditColorsCommand(
            this->pal, this->selectedFirstColorIndex, this->selectedLastColorIndex, color, color);
        QObject::connect(command, &EditColorsCommand::modified, this, &PaletteWidget::modify);

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
    EditColorsCommand *command = new EditColorsCommand(
        this->pal, this->selectedFirstColorIndex, this->selectedLastColorIndex, color, colorEnd);
    QObject::connect(command, &EditColorsCommand::modified, this, &PaletteWidget::modify);

    this->undoStack->push(command);
}

void PaletteWidget::on_colorClearPushButton_clicked()
{
    this->initStopColorPicking();

    QColor undefinedColor = QColor(Config::getPaletteUndefinedColor());

    // Build color editing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    auto *command = new EditColorsCommand(
        this->pal, this->selectedFirstColorIndex, this->selectedLastColorIndex, undefinedColor, undefinedColor);
    QObject::connect(command, &EditColorsCommand::modified, this, &PaletteWidget::modify);

    this->undoStack->push(command);
}

void PaletteWidget::on_translationIndexLineEdit_returnPressed()
{
    quint8 index = ui->translationIndexLineEdit->text().toUInt();

    if (index < D1PAL_COLORS) {
        // New translations
        QList<quint8> newTranslations;
        for (int i = this->selectedFirstColorIndex; i <= this->selectedLastColorIndex; i++)
            newTranslations.append(index);

        // Build translation editing command and connect it to the current palette widget
        // to update the PAL/TRN and CEL views when undo/redo is performed
        EditTranslationsCommand *command = new EditTranslationsCommand(
            this->trn, this->selectedFirstColorIndex, this->selectedLastColorIndex, newTranslations);
        QObject::connect(command, &EditTranslationsCommand::modified, this, &PaletteWidget::modify);

        this->undoStack->push(command);
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
    emit this->colorPicking_started();
}

void PaletteWidget::on_translationClearPushButton_clicked()
{
    this->initStopColorPicking();

    // Build translation clearing command and connect it to the current palette widget
    // to update the PAL/TRN and CEL views when undo/redo is performed
    auto *command = new ClearTranslationsCommand(
        this->trn, this->selectedFirstColorIndex, this->selectedLastColorIndex);
    QObject::connect(command, &ClearTranslationsCommand::modified, this, &PaletteWidget::modify);

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
