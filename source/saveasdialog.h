#pragma once

#include <QDialog>
#include <QString>

class D1Hero;

enum class SAVE_HERO_TYPE {
    AUTODETECT,
    DIABLO_HERO,
    HELLFIRE_HERO,
};

class SaveAsParam {
public:
    QString filePath;
    SAVE_HERO_TYPE heroType = SAVE_HERO_TYPE::AUTODETECT;
    bool autoOverwrite = false;
};

namespace Ui {
class SaveAsDialog;
}

class SaveAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveAsDialog(QWidget *parent);
    ~SaveAsDialog();

    void initialize(D1Hero *hero);

private slots:
    void on_outputFileBrowseButton_clicked();
    void on_saveButton_clicked();
    void on_saveCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::SaveAsDialog *ui;
    D1Hero *hero = nullptr;
};
