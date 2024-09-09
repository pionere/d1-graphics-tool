#pragma once

#include <QDialog>
#include <QString>

enum class OPEN_HERO_TYPE {
    AUTODETECT,
    DIABLO_HERO,
    HELLFIRE_HERO,
};

enum class OPEN_HERO_CLASS {
    AUTODETECT,
    WARRIOR,
    ROGUE,
    SORCERER,
    MONK,
    BARD,
    BARBARIAN,
};

class OpenAsParam {
public:
    QString filePath;
    OPEN_HERO_TYPE heroType = OPEN_HERO_TYPE::AUTODETECT;
    OPEN_HERO_CLASS heroClass = OPEN_HERO_CLASS::AUTODETECT;
};

namespace Ui {
class OpenAsDialog;
}

class OpenAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OpenAsDialog(QWidget *parent);
    ~OpenAsDialog();

    void initialize();

private:
    void updateFields();

private slots:
    void on_inputFileBrowseButton_clicked();
    void on_openButton_clicked();
    void on_openCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::OpenAsDialog *ui;
};
