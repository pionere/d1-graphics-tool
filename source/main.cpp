#include <QApplication>
#include <QFile>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Config::loadConfiguration();

    const char *qssName = ":/D1GraphicsTool.qss";
    QFile file(qssName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open " << qssName;
        return -1;
    }
    QString styleSheet = QTextStream(&file).readAll();
    a.setStyleSheet(styleSheet);

    int result;

    {
        MainWindow w;
        w.setWindowTitle("Diablo 1 Graphics Tool");
        w.show();

        result = a.exec();
    }

    Config::storeConfiguration();

    return result;
}
