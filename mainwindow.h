#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include "videopipeline.h"
#include "externalresources.h"
#include "mandelbrotpipeline.h"
#include "cpu/cpuinfo.h"

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{

    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();

private slots:
    void on_buttonVideodemo_toggled(bool checked);
    void on_cbYUVToRGB_clicked(bool checked);

    void updateVideoDemoState(bool active);
    void updateMandelbrotDemoState(bool active);
    void hideProgrammingMetrics();
    void showProgrammingMetrics(int node, const char* name, unsigned int size, unsigned int microseconds);
    void showVideoStats(unsigned int frames,  unsigned int milliseconds);
    void mustHaveAccellYUVtoRGB(bool checked);

    void on_buttonMandelbrotDemo_toggled(bool checked);

    void updateCpuStats();

    void on_node5_overlay_linkActivated(const QString &link);

private:
    Ui::MainWindow*   ui;
    CpuInfo cpuInfo;
    QTimer cpuStatsTimer;
    VideoPipeline video;
    MandelbrotPipeline mandelbrot;
    ExternalResources externals;

    QLabel *getPrRegion(int id);
    void updateFloorplan();
    void externalResourceEnable(int id, bool active);
};

#endif // MAINWINDOW_H
