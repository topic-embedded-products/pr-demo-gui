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
class FractalFrame;
class VideoFrame;
class TopPanel;
class FloorplanFrame;
}

class IIOTempSensor;
class SupplyCurrentSensor;
class QScrollArea;

class MainWindow : public QMainWindow
{

    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void showIn(const QRect &rec);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void updateVideoDemoState(bool active);
    void updateMandelbrotDemoState(bool active);
    void hideProgrammingMetrics();
    void showProgrammingMetrics(int node, const char* name, unsigned int size, unsigned int microseconds);
    void showVideoStats(unsigned int frames,  unsigned int milliseconds);
    void showMandelbrotStats(unsigned int frames,  unsigned int milliseconds);
    void updateCpuStats();

    void buttonVideodemo_toggled(bool checked);
    void buttonMandelbrotDemo_toggled(bool checked);
    void mandelbrotClicked(QMouseEvent *event);
    void prNodeLinkActivated(const QString &link);
    void btnPresetA_clicked();
    void btnPresetB_clicked();
    void btnPresetC_clicked();

private:
    Ui::MainWindow*   ui;
    Ui::FractalFrame* ui_fractal;
    Ui::VideoFrame*   ui_video;
    Ui::FloorplanFrame* ui_floorplan;
    Ui::TopPanel*     ui_toppanel;
    QWidget*    fractalWidget;
    QFrame*    videoWidget;
    QScrollArea* floorplanWidget;
    QWidget*    toppanelWidget;
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
