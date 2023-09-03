#pragma once

#include <QWidget>

class D1Pal;
class TrnGenerateDialog;

namespace Ui {
class TrnGeneratePalEntryWidget;
} // namespace Ui

class TrnGeneratePalEntryWidget : public QWidget {
    Q_OBJECT

public:
    explicit TrnGeneratePalEntryWidget(TrnGenerateDialog *parent, D1Pal *pal, bool delPal);
    ~TrnGeneratePalEntryWidget();

    D1Pal *getPalette() const;

private slots:
    void on_deletePushButtonClicked();
    void on_paletteFileBrowseButton_clicked();

private:
    Ui::TrnGeneratePalEntryWidget *ui;
    TrnGenerateDialog *view;
    D1Pal *pal;
    bool delPal;
};
