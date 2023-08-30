#pragma once

#include <QWidget>

class TrnGenerateDialog;
struct GenerateTrnColor;

namespace Ui {
class TrnGenerateColEntryWidget;
} // namespace Ui

class TrnGenerateColEntryWidget : public QWidget {
    Q_OBJECT

public:
    explicit TrnGenerateColEntryWidget(TrnGenerateDialog *parent);
    ~TrnGenerateColEntryWidget();

    GenerateTrnColor getTrnColor() const;

private slots:
    void on_deletePushButtonClicked();

private:
    Ui::TrnGenerateColEntryWidget *ui;
    TrnGenerateDialog *view;
};
