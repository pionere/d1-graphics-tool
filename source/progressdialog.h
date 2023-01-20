#pragma once

#include <QWidget>

class ProgressDialog {
private:

public:
    static void intialize(QWidget *window);

    static void show(const QString &label, int maxValue);
    static void hide();

    static bool wasCanceled();
    static void incValue();
};
