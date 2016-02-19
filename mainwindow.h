#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include "audiopipeline.h"
#include "videopipeline.h"

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
    void on_buttonAudioDemo_toggled(bool checked);
    void on_buttonMandelbrotDemo_toggled(bool checked);

    void updateVideoDemoState(bool active);
    void updateAudioDemoState(bool active);
    void hideProgrammingMetrics();
    void showProgrammingMetrics(int node, const char* name, unsigned int size, unsigned int microseconds);
    void showVideoStats(unsigned int frames,  unsigned int milliseconds);

    void mustHaveAccellYUVtoRGB(bool checked);

    void on_cbYUVToRGB_clicked(bool checked);

private:
    Ui::MainWindow*   ui;
    AudioPipeline audio;
    VideoPipeline video;

    QLabel *getPrRegion(int id);
    void updateFloorplan();
};

#endif // MAINWINDOW_H
