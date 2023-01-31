#pragma once

#include <QDialog>
#include <QString>

class D1Gfx;
class D1Min;
class D1Til;
class D1Sol;
class D1Amp;
class D1Tmi;

enum class SAVE_CLIPPED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

enum class SAVE_UPSCALED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

class SaveAsParam {
public:
    QString celFilePath;
    QString tilFilePath;
    QString minFilePath;
    QString solFilePath;
    QString ampFilePath;
    QString tmiFilePath;
    quint16 groupNum = 0;
    SAVE_CLIPPED_TYPE clipped = SAVE_CLIPPED_TYPE::AUTODETECT;
    SAVE_UPSCALED_TYPE upscaled = SAVE_UPSCALED_TYPE::AUTODETECT;
    bool autoOverwrite = false;
};

namespace Ui {
class SaveAsDialog;
}

class SaveAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveAsDialog(QWidget *parent = nullptr);
    ~SaveAsDialog();

    void initialize(D1Gfx *gfx, D1Min *min, D1Til *til, D1Sol *sol, D1Amp *amp, D1Tmi *tmi);

private slots:
    void on_outputCelFileBrowseButton_clicked();
    void on_outputMinFileBrowseButton_clicked();
    void on_outputTilFileBrowseButton_clicked();
    void on_outputSolFileBrowseButton_clicked();
    void on_outputAmpFileBrowseButton_clicked();
    void on_outputTmiFileBrowseButton_clicked();
    void on_saveButton_clicked();
    void on_saveCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event);

private:
    Ui::SaveAsDialog *ui;
    D1Gfx *gfx;
    bool isTileset;
};
