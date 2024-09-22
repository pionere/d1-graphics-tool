#pragma once

#include <QDirIterator>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPointer>
#include <QStyle>
#include <QUndoCommand>
#include <QUndoStack>
#include <QWidget>

#include "d1pal.h"
#include "d1trn.h"

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

class EditPaletteCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditPaletteCommand(D1Pal *pal, quint8 startColorIndex, quint8 endColorIndex, QColor newColorStart, QColor newColorEnd);
    explicit EditPaletteCommand(D1Pal *pal, quint8 startColorIndex, quint8 endColorIndex, const QList<QColor> &modColors);
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
    explicit EditTranslationCommand(D1Trn *trn, quint8 startColorIndex, quint8 endColorIndex, const std::vector<quint8> *newTranslations);
    explicit EditTranslationCommand(D1Trn *trn, const std::vector<std::pair<quint8, quint8>> &modTranslations);
    ~EditTranslationCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Trn> trn;
    std::vector<std::pair<quint8, quint8>> modTranslations;
};

namespace Ui {
class PaletteWidget;
} // namespace Ui

class PaletteWidget;

class PaletteScene : public QGraphicsScene {
    Q_OBJECT

public:
    PaletteScene(PaletteWidget *view);

    static int getColorIndexFromCoordinates(QPointF coordinates);
    static QRectF getColorCoordinates(quint8 index);

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

signals:
    void framePixelClicked(quint16, quint16);

private:
    PaletteWidget *view;
};

class PaletteWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaletteWidget(QWidget *parent, QUndoStack *undoStack, QString title);
    ~PaletteWidget();

    void setPal(D1Pal *p);
    void setTrn(D1Trn *t);
    bool isTrnWidget();

    void initialize(D1Pal *p);
    void initialize(D1Trn *t);

    void initializeUi();
    // void initializePathComboBox();
    void initializeDisplayComboBox();

    std::pair<int, int> getCurrentSelection() const;
    void checkTranslationsSelection(const std::vector<quint8> &indices);

    // color selection handlers
    void startColorSelection(int colorIndex);
    void changeColorSelection(int colorIndex);
    void changeColorSelection(int dir, bool extend);
    void finishColorSelection();

    void startTrnColorPicking(bool single);
    void stopTrnColorPicking();

    void updatePathComboBoxOptions(const QList<QString> &options, const QString &selectedOption);
    // void refreshPathComboBox();
    void refreshColorLineEdit();
    void refreshIndexLineEdit();
    void refreshTranslationIndexLineEdit();
    void patchTrn();

    void updateFields();
    void modify();
    void refresh();

signals:
    void pathSelected(QString path);
    void colorsSelected(const std::vector<quint8> &indices);
    void colorsPicked(const std::vector<quint8> &indices);

    void colorPicking_started(bool single);
    void colorPicking_stopped();

    void refreshed();

private:
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

    void keyPressEvent(QKeyEvent *event) override;

private:
    QUndoStack *undoStack;
    Ui::PaletteWidget *ui;
    bool isTrn;

    PaletteScene scene = PaletteScene(this);

    int selectedFirstColorIndex = 0;
    int selectedLastColorIndex = 0;
    int prevSelectedColorIndex = 0;

    bool pickingTranslationColor = false;

    D1Pal *pal;
    D1Trn *trn;
};
