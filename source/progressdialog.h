#pragma once

#include <QProgressDialog>
#include <QWidget>

class ProgressDialog : public QProgressDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog() = default;

    static void show(const QString &label, int maxValue);
    static void hide();

    static bool wasCanceled();
    static void incValue();
};
