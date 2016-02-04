#include "mainwindow.h"
#include <QApplication>
#include "mousemonitor.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MouseMonitor mousemon;
    a.installEventFilter(&mousemon);
    MainWindow w;
    w.showFullScreen();
    return a.exec();
}
