#pragma once

#include <QDirIterator>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPointer>
#include <QStyle>
#include <QUndoCommand>
#include <QUndoStack>
#include <QWidget>

#include "celview.h"
#include "d1pal.h"
#include "d1palhits.h"
#include "d1trn.h"
#include "levelcelview.h"

#define PALETTE_WIDTH 192
#define PALETTE_COLORS_PER_LINE 16
#define PALETTE_COLOR_SPACING 1
#define PALETTE_SELECTION_WIDTH 2

enum class PWIDGET_CALLBACK_TYPE {
    PWIDGET_CALLBACK_NEW,
    PWIDGET_CALLBACK_OPEN,
    PWIDGET_CALLBACK_SAVE,
    PWIDGET_CALLBACK_SAVEAS,
    PWIDGET_CALLBACK_CLOSE,
};

namespace Ui {
class PaletteWidget;
} // namespace Ui

class EditPaletteCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditPaletteCommand(D1Pal *pal, quint8 startColorIndex, quint8 endColorIndex, QColor newColorStart, QColor newColorEnd, QUndoCommand *parent = nullptr);
    explicit EditPaletteCommand(D1Pal *pal, quint8 startColorIndex, quint8 endColorIndex, QList<QColor> modColors, QUndoCommand *parent = nullptr);
    ~EditPaletteCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Pal> pal;
    quint8 startColorIndex;
    quint8 endColorIndex;
    QList<QColor> modColors;
};

class EditTranslationCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditTranslationCommand(D1Trn *trn, quint8 startColorIndex, quint8 endColorIndex, QList<quint8> *newTranslations, QUndoCommand *parent = nullptr);
    ~EditTranslationCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Trn> trn;
    quint8 startColorIndex;
    quint8 endColorIndex;
    QList<quint8> modTranslations;
};

class PaletteScene : public QGraphicsScene {
    Q_OBJECT

public:
    PaletteScene(QWidget *view);

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void framePixelClicked(quint16, quint16);

private:
    QWidget *view;
};

class PaletteWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaletteWidget(QUndoStack *undoStack, QString title);
    ~PaletteWidget();

    void setPal(D1Pal *p);
    void setTrn(D1Trn *t);
    bool isTrnWidget();

    void initialize(D1Pal *p, CelView *c, LevelCelView *lc, D1PalHits *ph);
    void initialize(D1Pal *p, D1Trn *t, CelView *c, LevelCelView *lc, D1PalHits *ph);

    void initializeUi();
    void initializePathComboBox();
    void initializeDisplayComboBox();

    void selectColor(const D1GfxPixel &pixel);
    D1GfxPixel getCurrentColor(unsigned counter) const;
    void checkTranslationsSelection(QList<quint8> indices);

    void addPath(const QString &path, const QString &name);
    void removePath(QString path);
    QString getSelectedPath() const;

    // color selection handlers
    void startColorSelection(int colorIndex);
    void changeColorSelection(int colorIndex);
    void finishColorSelection();

    void startTrnColorPicking(bool single);
    void stopTrnColorPicking();

    void refreshPathComboBox();
    void refreshColorLineEdit();
    void refreshIndexLineEdit();
    void refreshTranslationIndexLineEdit();

    void modify();
    void refresh();

signals:
    void pathSelected(QString path);
    void colorsSelected(QList<quint8> indices);
    void changeColor(quint8 startColorIndex, quint8 endColorIndex, D1GfxPixel pixel, bool all);

    void colorPicking_started(bool single);
    void colorPicking_stopped();

    void modified();
    void refreshed();

private:
    QPushButton *addButton(QStyle::StandardPixmap type, QString tooltip, void (PaletteWidget::*callback)(void));
    // Display functions
    void displayColors();
    void displaySelection();

    void initStopColorPicking();

public slots:
    void ShowContextMenu(const QPoint &pos);

private slots:
    // Due to a bug in Qt these functions can not follow the naming conventions
    // if they follow, the application is going to vomit warnings in the background (maybe only in debug mode)
    void on_newPushButtonClicked();
    void on_openPushButtonClicked();
    void on_savePushButtonClicked();
    void on_saveAsPushButtonClicked();
    void on_closePushButtonClicked();

    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();

    void on_pathComboBox_activated(int index);
    void on_displayComboBox_activated(int index);
    void on_colorLineEdit_returnPressed();
    void on_colorLineEdit_escPressed();
    void on_colorPickPushButton_clicked();
    void on_colorClearPushButton_clicked();
    void on_translationIndexLineEdit_returnPressed();
    void on_translationIndexLineEdit_escPressed();
    void on_translationPickPushButton_clicked();
    void on_translationClearPushButton_clicked();
    void on_monsterTrnPushButton_clicked();

    void keyPressEvent(QKeyEvent *event);

private:
    QUndoStack *undoStack;
    Ui::PaletteWidget *ui;
    bool isTrn;

    CelView *celView;
    LevelCelView *levelCelView;

    PaletteScene *scene;

    int selectedFirstColorIndex = 0;
    int selectedLastColorIndex = 0;
    int prevSelectedColorIndex = 0;

    bool pickingTranslationColor = false;

    D1Pal *pal;
    D1Trn *trn;

    D1PalHits *palHits;

    QMap<QString, QString> paths;
};
