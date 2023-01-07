#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QString>

class D1Gfx;
class D1Min;
class D1Til;
class D1Sol;
class D1Amp;

enum class SAVE_CLIPPING_TYPE {
    CLIPPED_AUTODETECT,
    CLIPPED_TRUE,
    CLIPPED_FALSE,
};

class SaveAsParam {
public:
    QString celFilePath;
    QString tilFilePath;
    QString minFilePath;
    QString solFilePath;
    QString ampFilePath;
    quint16 groupNum = 0;
    SAVE_CLIPPING_TYPE clipped = SAVE_CLIPPING_TYPE::CLIPPED_AUTODETECT;
};

namespace Ui {
class SaveAsDialog;
}

class SaveAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveAsDialog(QWidget *parent = nullptr);
    ~SaveAsDialog();

    void initialize(QJsonObject *cfg, D1Gfx *gfx, D1Min *min, D1Til *til, D1Sol *sol, D1Amp *amp);

private:
    void update();

private slots:
    void on_outputCelFileBrowseButton_clicked();
    void on_outputMinFileBrowseButton_clicked();
    void on_outputTilFileBrowseButton_clicked();
    void on_outputSolFileBrowseButton_clicked();
    void on_outputAmpFileBrowseButton_clicked();
    void on_saveButton_clicked();
    void on_saveCancelButton_clicked();

private:
    Ui::SaveAsDialog *ui;
    QJsonObject *configuration;
    D1Gfx *gfx;
    bool isTileset;
};
