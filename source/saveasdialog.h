#pragma once

#include <QDialog>
#include <QString>

class D1Hero;

enum class SAVE_HELLFIRE_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

class SaveAsParam {
public:
    QString filePath;
    SAVE_HELLFIRE_TYPE hellfireHero = SAVE_HELLFIRE_TYPE::AUTODETECT;
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
