#include "cppview.h"

#include <algorithm>

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "celview.h"
#include "config.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "pushbuttonwidget.h"
#include "ui_cppview.h"

CppView::CppView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CppView())
{
    this->ui->setupUi(this);

    // setup context menu
    // this->setContextMenuPolicy(Qt::CustomContextMenu);
    // QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

CppView::~CppView()
{
    delete ui;
}

void CppView::initialize(D1Cpp *c)
{
    this->cpp = c;

    int n = c->getTableCount();
    if (n != 0) {
        for (int i = 0; i < n; i++) {
            D1CppTable *table = c->getTable(i);
            this->ui->tablesComboBox->addItem(table->getName(), i);
        }
        this->ui->tablesComboBox->setCurrentIndex(0);
    }

    this->updateFields();
}

// Displaying CPP file path information
void CppView::updateLabel()
{
    CppView::setLabelContent(this->ui->cppLabel, this->cpp->getFilePath(), this->cpp->isModified());
}

void CppView::updateFields()
{
    this->updateLabel();
}

void CppView::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void CppView::dragMoveEvent(QDragMoveEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        QString filePath = url.toLocalFile();
        // add PCX support
        if (filePath.toLower().endsWith(".cpp")) {
            event->acceptProposedAction();
            return;
        }
    }
}

void CppView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    dMainWindow().openFiles(filePaths);
}
