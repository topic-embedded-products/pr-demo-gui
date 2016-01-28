#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filterprogrammer.h"

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

private:
    void SetFilterStatus(FilterProgrammer::Filters filter, bool enabled);
    void showOverlay(FilterProgrammer::PrRegion prRegion, const QColor& color);
    void hideOverlay(FilterProgrammer::PrRegion prRegion);

    Ui::MainWindow*   ui;
    FilterProgrammer  programmer;
    QMap<FilterProgrammer::Filters, QColor> filterColorMap;

    QString getOverlayBackgroundColor(const QColor& color);
};

#endif // MAINWINDOW_H
