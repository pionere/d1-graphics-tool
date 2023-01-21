#include <QApplication>
#include <QFile>

#include "config.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Config::loadConfiguration();

    { // load style-sheet
        const char *qssName = ":/D1GraphicsTool.qss";
        QFile file(qssName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << tr("Failed to open %1").arg(qssName);
            return -1;
        }
        QString styleSheet = QTextStream(&file).readAll();
        a.setStyleSheet(styleSheet);
    }

    int result;
    { // run the application
        MainWindow w;
        w.show();

        result = a.exec();
    }

    Config::storeConfiguration();

    return result;
}
