#include "mainwindow.h"
#include <QApplication>
#include <thread>
#include <QSharedMemory>

int main(int argc, char *argv[])
{
    QSharedMemory shared("name");
    if (shared.attach())
    {
            return 0;
    }
    shared.create(1);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

