#pragma once

#include <QDialog>

enum class IMPORT_FILE_TYPE {
    AUTODETECT,
    CEL,
    CL2,
    DUNGEON,
};

typedef struct ImportParam {
    QString filePath;
    IMPORT_FILE_TYPE fileType = IMPORT_FILE_TYPE::AUTODETECT;
} ImportParam;

class PaletteWidget;

namespace Ui {
class ImportDialog;
}

class ImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportDialog(QWidget *parent);
    ~ImportDialog();

    void initialize(bool dunMode);

private:
    void updateFields();

private slots:
    void on_inputFileBrowseButton_clicked();

    void on_fileTypeCELRadioButton_toggled(bool checked);
    void on_fileTypeCL2RadioButton_toggled(bool checked);
    void on_fileTypeDUNRadioButton_toggled(bool checked);
    void on_fileTypeAutoRadioButton_toggled(bool checked);

    void on_importButton_clicked();
    void on_importCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::ImportDialog *ui;

    bool dunMode;
};
