#include "checkgfxsetstaskdialog.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSet>

#include "d1cel.h"
#include "d1cl2.h"
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

    /*ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Checking assets..."), 0, PAF_NONE);

    CheckGfxsetsTaskDialog::runTask(params);

    ProgressDialog::done();*/
    std::function<void()> func = [params]() {
        CheckGfxsetsTaskDialog::runTask(params);
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Checking assets..."), 0, PAF_OPEN_DIALOG, std::move(func));
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
        QFileInfo sfi(sPath);
        if (sfi.isDir()) {
            if (params.recursive) {
                CheckGfxsetsTaskParam pm = params;
                pm.folder = sPath;
                CheckGfxsetsTaskDialog::runTask(pm);
            }
        } else {
            QFileInfo xfi(sPath);
            QString extension = xfi.suffix();
            D1Gfx* gfx = new D1Gfx();
            D1Gfxset* gfxset = new D1Gfxset(gfx);
            OpenAsParam pm;
            if (gfxset->load(sPath, pm, true)) {
                QString gfxsetName = gfx->getFilePath();
                gfxsetName.chop(extension.length() + 1 + (gfxset->getType() == D1GFX_SET_TYPE::Player ? 1 : 0));
                gfxsetName[gfxsetName.length() - 1] = 'x';

                QPair<int, QString> progress;
                progress.first = -1;
                progress.second = tr("Inconsistencies in the graphics of the '%1' gfx-set:").arg(gfxsetName);

                dProgress() << progress;
                bool result = gfxset->check(nullptr, params.multiplier);
                if (!result) {
                    progress.second = tr("No inconsistency detected in the '%1' gfx-set.").arg(gfxsetName);
                    dProgress() << progress;
                }

                for (int i = 0; i < gfxset->getGfxCount(); i++) {
                    checked.insert(gfxset->getGfx(i)->getFilePath());
                }
            } else {
                bool loaded = false;
                if (extension.toLower() == "cl2") {
                    loaded = D1Cl2::load(*gfx, sPath, pm);
                    if (!loaded) {
                        dProgressErr() << tr("Failed loading CL2: %1.").arg(sPath);
                    }
                } else if (extension.toLower() == "cel") {
                    loaded = D1Cel::load(*gfx, sPath, pm);
                    if (!loaded) {
                        dProgressErr() << tr("Failed loading CEL: %1.").arg(sPath);
                    }
                }
                if (loaded) {
                    QPair<int, QString> progress;
                    progress.first = -1;
                    progress.second = tr("Inconsistencies in '%1':").arg(sPath);

                    dProgress() << progress;
                    bool result = gfx->check(params.multiplier);
                    if (!result) {
                        progress.second = tr("No inconsistency detected in '%1'.").arg(sPath);
                        dProgress() << progress;
                    }
                }
            }
            delete gfxset;
            delete gfx;
        }
    }
}
