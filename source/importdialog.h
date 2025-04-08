#pragma once

#include <QDialog>

enum class IMPORT_FILE_TYPE {
    AUTODETECT,
    MIN,
    TIL,
    SLA,
    TLA,
    SCEL,
    DUNGEON,
    CEL,
    CL2,
    FONT,
    // unsupported types
    SMK,
    PCX,
    TBL,
    CPP,
};

typedef struct ImportParam {
    QString filePath;
    IMPORT_FILE_TYPE fileType = IMPORT_FILE_TYPE::AUTODETECT;

    int fontSize;
    int fontRangeFrom;
    int fontRangeTo;
    int fontColor;
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

    void initialize(bool dunMode, const PaletteWidget *palWidget);

private:
    void updateFields();

private slots:
    void on_inputFileBrowseButton_clicked();

    void on_fileTypeMINRadioButton_toggled(bool checked);
    void on_fileTypeTILRadioButton_toggled(bool checked);
    void on_fileTypeSLARadioButton_toggled(bool checked);
    void on_fileTypeTLARadioButton_toggled(bool checked);
    void on_fileTypeDUNRadioButton_toggled(bool checked);
    void on_fileTypeSCELRadioButton_toggled(bool checked);
    void on_fileTypeCELRadioButton_toggled(bool checked);
    void on_fileTypeCL2RadioButton_toggled(bool checked);
    void on_fileTypeFontRadioButton_toggled(bool checked);
    void on_fileTypeAutoRadioButton_toggled(bool checked);

    void on_fontSizeEdit_returnPressed();
    void on_fontSizeEdit_escPressed();
    void on_fontRangeFromEdit_returnPressed();
    void on_fontRangeFromEdit_escPressed();
    void on_fontRangeToEdit_returnPressed();
    void on_fontRangeToEdit_escPressed();
    void on_fontRangeHexCheckBox_clicked();
    void on_fontColorEdit_returnPressed();
    void on_fontColorEdit_escPressed();

    void on_importButton_clicked();
    void on_importCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::ImportDialog *ui;

    bool dunMode;
    int font_color;
    int font_size = 22;
    int font_rangeFrom = 0;
    int font_rangeTo = 255;
    bool font_rangeHex = false;
};
