#include <QApplication>
#include <QFile>

#include "config.h"
#include "mainwindow.h"

#include "torch/torch.h"
#include "dungeon/interfac.h"

struct Net : torch::nn::Module {
    Net() {
        // fc1 = register_module("fc1", torch::nn::Linear(4, 8));
        // fc2 = register_module("fc2", torch::nn::Linear(8, 3));
    }
#if 0
    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(fc1->forward(x));
        x = torch::softmax(fc2->forward(x), /*dim=*/1);
        return x;
    }
    torch::nn::Linear fc1{nullptr};
    torch::nn::Linear fc2{nullptr};
#endif
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
    Config::loadConfiguration();

    { // load style-sheet
        const char *qssName = ":/D1GraphicsTool.qss";
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
#if 1
        //try {
        {
            Net net;
            // example inpuit batch of 2 samples, each with 4 features 
//            torch::Tensor input = torch::rand({ 2,4 });
            // forward pass
//            torch::Tensor output = net.forward(input);

            torch::save(net, "model.pt");

//            Net loaded_net;
//            torch::load(loaded_net, "model.pt");

        //} catch (const c10::Error& e) {
        //    LogErrorF("failed c10");
        }
//        LogErrorF("success");
#endif
        w.show();

        if (argc > 1) {
            w.openArgFile(argv[argc - 1]);
        }
        result = a.exec();
    }

    Config::storeConfiguration();

    return result;
}
