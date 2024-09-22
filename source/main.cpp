#include <QApplication>
#include <QFile>

#include "config.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
    Config::loadConfiguration();

    { // load style-sheet
        const char *qssName = ":/D1HeroPlanner.qss";
        QFile file(qssName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << QApplication::tr("Failed to read file: %1.").arg(qssName);
            return -1;
        }
        QString styleSheet = QTextStream(&file).readAll();
        a.setStyleSheet(styleSheet);
    }

    int result;
    { // run the application
        MainWindow w = MainWindow();
        w.show();

        if (argc > 1) {
            w.openArgFile(argv[argc - 1]);
        }
        result = a.exec();
    }

    Config::storeConfiguration();

    return result;
}
