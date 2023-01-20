#pragma once

#include <QWidget>

class ProgressDialog {
public:
    static void initialize(QWidget *window);

    static void show(const QString &label, int maxValue);
    static void hide();

    static bool wasCanceled();
    static void incValue();
};
