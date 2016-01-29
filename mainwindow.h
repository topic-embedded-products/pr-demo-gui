#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filterprogrammer.h"
#include "dyplocontext.h"
#include "dyplorouter.h"

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
    void on_buttonGrayscale_toggled(bool checked);

    void on_buttonLaplacian_toggled(bool checked);

    void on_buttonLowPass_toggled(bool checked);

    void on_buttonHighPass_toggled(bool checked);

    void on_buttonFFT_toggled(bool checked);

    void on_buttonMandelbrot_toggled(bool checked);

    void hideProgrammingMetrics();

private:
    void showProgrammingMetrics(EPrRegion programRegion, quint32 programTimeMs);
    void setFilterStatus(EFilters filter, bool enabled);
    void showOverlay(EPrRegion prRegion, const QColor& color);
    void hideOverlay(EPrRegion prRegion);

    Ui::MainWindow*   ui;

    FilterProgrammer  filterProgrammer;
    DyploRouter       dyploRouter;

    QMap<EFilters, QColor> filterColorMap;

    QString getOverlayBackgroundColor(const QColor& color);
};

#endif // MAINWINDOW_H
