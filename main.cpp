#include "mainwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include "mousemonitor.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MouseMonitor mousemon;
    a.installEventFilter(&mousemon);
    MainWindow w;
    QRect rec = QApplication::desktop()->availableGeometry();
    w.showIn(rec);
    return a.exec();
}
