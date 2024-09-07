#pragma once

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QLabel>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "d1hro.h"
#include "d1pal.h"
#include "pushbuttonwidget.h"

#define CEL_SCENE_SPACING 8
#define CEL_SCENE_MARGIN 0

namespace Ui {
class CelView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

typedef enum _mouse_click_flags {
    FIRST_CLICK  = 1 << 0,
    DOUBLE_CLICK = 1 << 1,
    SHIFT_CLICK  = 1 << 2,
} _mouse_click_flags;

class CelScene : public QGraphicsScene {
    Q_OBJECT

public:
    CelScene(QWidget *view);

private:
    void mouseEvent(QGraphicsSceneMouseEvent *event, int flags);
    void mouseHoverEvent(QGraphicsSceneMouseEvent *event);

private slots:
    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    //void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    //void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    //void dropEvent(QGraphicsSceneDragDropEvent *event) override;

signals:
    // void framePixelClicked(const QPoint &pos, int flags);
    // void framePixelHovered(const QPoint &pos);

private:
    bool panning = false;
    bool leftMousePressed = false;
    QPoint lastPos;
};

class CelView : public QWidget {
    Q_OBJECT

public:
    explicit CelView(QWidget *parent);
    ~CelView();

    void initialize(D1Pal *pal, D1Hero *hero, bool bottomPanelHidden);
    void setPal(D1Pal *pal);
    void setHero(D1Hero *hero);

    CelScene *getCelScene() const;
    int getCurrentFrameIndex() const;

    void framePixelClicked(const QPoint &pos, int flags);
    void framePixelHovered(const QPoint &pos) const;

    void displayFrame();
    void toggleBottomPanel();

    static void setLabelContent(QLabel *label, const QString &filePath, bool modified);

private:
    void updateFields();
    void updateLabel();
    void setGroupIndex(int groupIndex);
    bool framePos(QPoint &pos) const;

signals:
    void frameRefreshed();
    void palModified();

private slots:
    void on_framesGroupCheckBox_clicked();

    void on_heroNameEdit_returnPressed();
    void on_heroNameEdit_escPressed();
    void on_heroLevelEdit_returnPressed();
    void on_heroLevelEdit_escPressed();

    void on_heroDecLifeButton_clicked();
    void on_heroRestoreLifeButton_clicked();    

    //void dragEnterEvent(QDragEnterEvent *event) override;
    //void dragMoveEvent(QDragMoveEvent *event) override;
    //void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::CelView *ui;
    CelScene celScene = CelScene(this);

    D1Pal *pal;
    D1Hero *hero;
};
