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

class IIOTempSensor;
class SupplyCurrentSensor;

class MainWindow : public QMainWindow
{

    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void on_buttonVideodemo_toggled(bool checked);

    void updateVideoDemoState(bool active);
    void updateMandelbrotDemoState(bool active);
    void hideProgrammingMetrics();
    void showProgrammingMetrics(int node, const char* name, unsigned int size, unsigned int microseconds);
    void showVideoStats(unsigned int frames,  unsigned int milliseconds);
    void showMandelbrotStats(unsigned int frames,  unsigned int milliseconds);

    void on_buttonMandelbrotDemo_toggled(bool checked);

    void mandelbrotClicked(QMouseEvent *event);

    void updateCpuStats();

    void prNodeLinkActivated(const QString &link);

    void on_btnPresetA_clicked();

    void on_btnPresetB_clicked();

    void on_btnPresetC_clicked();

    void on_pbTabVideo_clicked();

    void on_pbTabFractal_clicked();

    void on_pbTabVideo_clicked(bool checked);

    void on_pbTabFractal_clicked(bool checked);

    void on_pbTabFPGA_clicked(bool checked);

private:
    Ui::MainWindow*   ui;
    CpuInfo cpuInfo;
    QTimer cpuStatsTimer;
    VideoPipeline video;
    MandelbrotPipeline mandelbrot;
    ExternalResources externals;
    QString programmingMetrics;
    IIOTempSensor* tempSensor;
    SupplyCurrentSensor* currentSensor;
    int updateStatsRobin;

    QLabel *getPrRegion(int id);
    void updateFloorplan();
    void externalResourceEnable(int id, bool active);
};

#endif // MAINWINDOW_H
