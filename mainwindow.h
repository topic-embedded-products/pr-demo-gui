#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include "filterprogrammer.h"
#include "dyplocontext.h"
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
private:
    void programmedDemo(EDemo prDemo, const QList<EPrRegion>& prRegionsUsed);
    void disabledDemo(EDemo prDemo);

    void showProgrammingMetrics(quint32 programTimeMs);

    // OLD implementation. Will be removed in future:
    //void setFilterStatus(EFilters filter, bool enabled);

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
