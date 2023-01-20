#pragma once

#include <QProgressDialog>
#include <QWidget>

class ProgressDialog : public QProgressDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog() = default;

    static void start(const QString &label, int maxValue);
    static void done();

    static bool wasCanceled();
    static void incValue();
};
