#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include "filterprogrammer.h"
#include "dyplorouter.h"
#include "microphonecapturethread.h"

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
    void hideProgrammingMetrics();

    void on_buttonVideodemo_toggled(bool checked);
    void on_buttonAudioDemo_toggled(bool checked);
    void on_buttonMandelbrotDemo_toggled(bool checked);

    void updateVideoDemoState(bool active);
    void showProgrammingMetrics(int node, const char* name, unsigned int size, unsigned int microseconds);

    void showVideoStats(unsigned int frames,  unsigned int milliseconds);
private:
    void programmedDemo(EDemo prDemo, const QList<EPrRegion>& prRegionsUsed);
    void disabledDemo(EDemo prDemo);

    QLabel* getPrRegion(EPrRegion prRegion);

    Ui::MainWindow*   ui;

    FilterProgrammer  filterProgrammer;
    DyploRouter       dyploRouter;

    VideoPipeline video;

    QMap<EDemo, QColor> demoColorMap;
    QMap<EDemo, QList<EPrRegion> > demoPrRegionsUsedMap;

    MicrophoneCaptureThread microphoneCaptureThread;

    QString getOverlayBackgroundColor(const QColor& color);


};

#endif // MAINWINDOW_H
