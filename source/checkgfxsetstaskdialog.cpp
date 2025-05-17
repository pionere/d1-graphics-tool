#include "checkgfxsetstaskdialog.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSet>

#include "d1gfx.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_checkgfxsetstaskdialog.h"

#include "dungeon/all.h"

CheckGfxsetsTaskDialog::CheckGfxsetsTaskDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CheckGfxsetsTaskDialog())
{
    this->ui->setupUi(this);
}

CheckGfxsetsTaskDialog::~CheckGfxsetsTaskDialog()
{
    delete ui;
}

void CheckGfxsetsTaskDialog::on_assetsFolderBrowseButton_clicked()
{
    QString dirPath = dMainWindow().folderDialog(tr("Select Assets Folder"));

    if (dirPath.isEmpty())
        return;

    this->ui->assetsFolderLineEdit->setText(dirPath);
}

void CheckGfxsetsTaskDialog::on_checkRunButton_clicked()
{
    CheckGfxsetsTaskParam params;
    params.folder = this->ui->assetsFolderLineEdit->text();
    params.multiplier = this->ui->multiplierLineEdit->text().toInt();
    params.recursive = this->ui->recursiveCheckBox->isChecked();

    this->close();

    /*ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Checking assets..."), 3, PAF_NONE);

    CheckGfxsetsTaskDialog::runTask(params);

    ProgressDialog::done();*/
    std::function<void()> func = [params]() {
        CheckGfxsetsTaskDialog::runTask(params);
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Checking assets..."), 3, PAF_NONE, std::move(func));
}

void CheckGfxsetsTaskDialog::on_checkCancelButton_clicked()
{
    this->close();
}

void CheckGfxsetsTaskDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}

void CheckGfxsetsTaskDialog::runTask(const CheckGfxsetsTaskParam &params)
{
    QFileInfo fi(params.folder);
    if (!fi.isDir()) {
        return;
    }
    QSet<QString> checked;
    QDirIterator it(params.folder, QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::AllDirs |  QDir::Files | QDir::Readable);
    while (it.hasNext()) {
        QString sPath = it.next();
        if (checked.contains(sPath))
            continue;
        LogErrorF("CheckGfxsetsTaskDialog %s", sPath.toLatin1().data());
        QFileInfo sfi(sPath);
        if (sfi.isDir()) {
            if (params.recursive) {
                CheckGfxsetsTaskParam pm = params;
                pm.folder = sPath;
                CheckGfxsetsTaskDialog::runTask(pm);
            }
        } else {
            D1Gfx* gfx = new D1Gfx();
            D1Gfxset* gfxset = new D1Gfxset(gfx);
            OpenAsParam pm;
            LogErrorF("CheckGfxsetsTaskDialog 1");
            if (gfxset->load(sPath, pm)) {
                LogErrorF("CheckGfxsetsTaskDialog 2");
                gfxset->check(nullptr, params.multiplier);
                LogErrorF("CheckGfxsetsTaskDialog 3");
                for (int i = 0; i < gfxset->getGfxCount(); i++) {
                    checked.insert(gfxset->getGfx(i)->getFilePath());
                }
            }
            LogErrorF("CheckGfxsetsTaskDialog 4");
            delete gfxset;
            delete gfx;
        }
    }
}
