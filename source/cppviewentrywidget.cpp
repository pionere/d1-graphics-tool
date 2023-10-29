#include "cppviewentrywidget.h"

#include <QApplication>
#include <QStyle>

#include "cppview.h"
#include "d1cpp.h"
#include "labelwidget.h"
#include "lineeditwidget.h"
#include "pushbuttonwidget.h"

#include "ui_cppviewentrywidget.h"

static int entryHorizontalMargin = 0;

CppViewEntryWidget::CppViewEntryWidget(CppView *parent)
    : QWidget(parent)
    , ui(new Ui::CppViewEntryWidget())
    , view(parent)
{
    ui->setupUi(this);
}

CppViewEntryWidget::~CppViewEntryWidget()
{
    delete ui;
}

int CppViewEntryWidget::baseHorizontalMargin()
{
    if (entryHorizontalMargin == 0) {
        int result = 8;
        LineEditWidget w = LineEditWidget(nullptr);
        QMargins qm = w.textMargins();
        result += qm.left() + qm.right();
        qm = w.contentsMargins();
        result += qm.left() + qm.right();
        entryHorizontalMargin = result;
    }
    return entryHorizontalMargin;
}

void CppViewEntryWidget::initialize(D1CppTable *t, int r, int c, int width)
{
    this->table = t;
    this->rowNum = r;
    this->columnNum = c;

    // clear the layout
    QLayoutItem *child;
    while ((child = this->ui->entryHorizontalLayout->takeAt(0)) != nullptr) {
        delete child->widget(); // delete the widget
        delete child;           // delete the layout item
    }

    QWidget *w;
    if (r == 0) {
        // header
        if (c == 0) {
            w = PushButtonWidget::addButton(this, QStyle::SP_FileDialogInfoView, tr("Show info"), view, &CppView::on_toggleInfoButton_clicked);
        } else {
            // standard header
            QString text = t->getHeader(c - 1);
            QFontMetrics fm = this->fontMetrics();
            if (fm.horizontalAdvance(text) > width) {
                w = PushButtonWidget::addButton(this, QStyle::SP_DialogHelpButton, text, this, &CppViewEntryWidget::on_headerButton_clicked);
            } else {
                w = new LabelWidget(text, this);
                ((LabelWidget *)w)->setFixedWidth(width);
            }
        }
    } else if (c == 0) {
        // leader
        w = new LabelWidget(this);
        ((LabelWidget *)w)->setFixedWidth(width);
        QString tooltip = t->getRowText(r);
        /*QString tooltip = t->getRowText(r - 1);
        QString postText = t->getRowText(r);
        if (!postText.isEmpty()) {
            if (!tooltip.isEmpty()) {
                tooltip += "</p><p style='white-space:pre'>";
            }
            tooltip += postText;
        }*/
        QString text = t->getLeader(r - 1);
        if (!tooltip.isEmpty()) {
            tooltip.prepend("<html><head/><body><p style='white-space:pre'>");
            tooltip.append("</p></body></html>");
            w->setToolTip(tooltip);
            text.prepend("<html><head/><body><i>");
            text.append("</i></body></html>");
        }
        ((LabelWidget *)w)->setText(text);
    } else {
        // standard entry
        w = new LineEditWidget(t->getRow(r - 1)->getEntry(c - 1)->getContent(), this);
        ((LineEditWidget *)w)->setMinimumWidth(width);
        w->setToolTip(QString("%1/%2").arg(t->getLeader(r - 1)).arg(t->getHeader(c - 1)));
        QObject::connect(w, SIGNAL(returnPressed()), this, SLOT(on_entryLineEdit_returnPressed()));
        // connect esc events of LineEditWidgets
        QObject::connect(w, SIGNAL(cancel_signal()), this, SLOT(on_entryLineEdit_escPressed()));
    }
    this->widget = w;
    this->ui->entryHorizontalLayout->addWidget(w);

    if (r != 0 && c != 0) {
        // standard entry
        QString text = t->getRow(r - 1)->getEntryText(c - 1);
        w = PushButtonWidget::addButton(this, QStyle::SP_MessageBoxInformation, text, this, &CppViewEntryWidget::on_infoButton_clicked);
        w->setVisible(false);
        this->ui->entryHorizontalLayout->addWidget(w);
    }
}

void CppViewEntryWidget::on_headerButton_clicked()
{

}

void CppViewEntryWidget::on_infoButton_clicked()
{

}

void CppViewEntryWidget::on_toggleInfoButton()
{
    QList<QWidget *> children = this->ui->entryHorizontalLayout->findChildren<QWidget *>();
    if (children.count() == 0) {
        QMessageBox::critical("Error", "No widget found");
        return;
    }
    if (children.count() == 2) {
        QWidget *w = children[1];
        w->setVisible(!w->isVisible());
    }
    if (this->rowNum == 0 && this->columnNum == 0) {
        PushButtonWidget *w = (PushButtonWidget *)children[0];
        QString showText = tr("Show info");
        if (w->text() == showText) {
            showText = tr("Hide info");
        }
        w->setText(showText);
    }
}

void CppViewEntryWidget::on_entryLineEdit_returnPressed()
{
    QString text = ((LineEditWidget *)this->widget)->text();
    this->view->setTableContent(this->rowNum, this->columnNum, text);
    this->on_entryLineEdit_escPressed();
}

void CppViewEntryWidget::on_entryLineEdit_escPressed()
{
    QString text = this->table->getRow(this->rowNum - 1)->getEntry(this->columnNum - 1)->getContent();
    ((LineEditWidget *)this->widget)->setText(text);
    this->widget->clearFocus();
}
