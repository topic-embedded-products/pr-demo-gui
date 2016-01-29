#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    filterProgrammer(DyploContext::getInstance()),
    dyploRouter(DyploContext::getInstance(), filterProgrammer)
{
    ui->setupUi(this);

    hideOverlay(FilterProgrammer::PrRegion1);
    hideOverlay(FilterProgrammer::PrRegion2);
    hideOverlay(FilterProgrammer::PrRegion3);
    hideOverlay(FilterProgrammer::PrRegion4);
    hideOverlay(FilterProgrammer::PrRegion5);
    hideOverlay(FilterProgrammer::PrRegion6);
    hideOverlay(FilterProgrammer::PrRegion7);

    filterColorMap[FilterProgrammer::FilterVideoGrayscale] = QColor(0,0,255);
    filterColorMap[FilterProgrammer::FilterVideoLaplacian] = QColor(0,0,255);
    filterColorMap[FilterProgrammer::FilterAudioFFT] = QColor(255,0,0);
    filterColorMap[FilterProgrammer::FilterAudioHighpass] = QColor(255,0,0);
    filterColorMap[FilterProgrammer::FilterAudioLowpass] = QColor(255,0,0);
    filterColorMap[FilterProgrammer::FilterMandelbrot] = QColor(0,255,0);
}

void MainWindow::showOverlay(FilterProgrammer::PrRegion prRegion, const QColor& color)
{
    switch (prRegion)
    {
    case FilterProgrammer::PrRegion1:
        ui->node1_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node1_overlay->show();
        break;
    case FilterProgrammer::PrRegion2:
        ui->node2_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node2_overlay->show();
        break;
    case FilterProgrammer::PrRegion3:
        ui->node3_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node3_overlay->show();
        break;
    case FilterProgrammer::PrRegion4:
        ui->node4_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node4_overlay->show();
        break;
    case FilterProgrammer::PrRegion5:
        ui->node5_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node5_overlay->show();
        break;
    case FilterProgrammer::PrRegion6:
        ui->node6_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node6_overlay->show();
        break;
    case FilterProgrammer::PrRegion7:
        ui->node7_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node7_overlay->show();
        break;
    }
}

void MainWindow::hideOverlay(FilterProgrammer::PrRegion prRegion)
{
    switch (prRegion)
    {
    case FilterProgrammer::PrRegion1:
        ui->node1_overlay->hide();
        break;
    case FilterProgrammer::PrRegion2:
        ui->node2_overlay->hide();
        break;
    case FilterProgrammer::PrRegion3:
        ui->node3_overlay->hide();
        break;
    case FilterProgrammer::PrRegion4:
        ui->node4_overlay->hide();
        break;
    case FilterProgrammer::PrRegion5:
        ui->node5_overlay->hide();
        break;
    case FilterProgrammer::PrRegion6:
        ui->node6_overlay->hide();
        break;
    case FilterProgrammer::PrRegion7:
        ui->node7_overlay->hide();
        break;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getOverlayBackgroundColor(const QColor& color)
{
    int red, green, blue = 0;
    color.getRgb(&red, &green, &blue);

    return QString("background-color: rgba(%1, %2, %3, 50\%);").arg(red).arg(green).arg(blue);
}

void MainWindow::on_buttonGrayscale_toggled(bool checked)
{
    setFilterStatus(FilterProgrammer::FilterVideoGrayscale, checked);
}

void MainWindow::setFilterStatus(FilterProgrammer::Filters filter, bool enabled)
{
    if (enabled)
    {
        FilterProgrammer::PrRegion programRegion;
        bool programmingSucceeded = filterProgrammer.ProgramFilter(filter, programRegion);
        if (programmingSucceeded)
        {
            // TODO: show performance metrics of programming
            Q_ASSERT(filterColorMap.contains(filter));
            showOverlay(programRegion, filterColorMap[filter]);
        }
    }
    else
    {
        FilterProgrammer::PrRegion disableRegion;
        bool disabledNode = filterProgrammer.DisableFilter(filter, disableRegion);
        if (disabledNode)
        {
            hideOverlay(disableRegion);
        }
    }
}

void MainWindow::on_buttonLaplacian_toggled(bool checked)
{
    setFilterStatus(FilterProgrammer::FilterVideoLaplacian, checked);
}

void MainWindow::on_buttonLowPass_toggled(bool checked)
{
    setFilterStatus(FilterProgrammer::FilterAudioLowpass, checked);
}

void MainWindow::on_buttonHighPass_toggled(bool checked)
{
    setFilterStatus(FilterProgrammer::FilterAudioHighpass, checked);
}

void MainWindow::on_buttonFFT_toggled(bool checked)
{
    setFilterStatus(FilterProgrammer::FilterAudioFFT, checked);
}

void MainWindow::on_buttonMandelbrot_toggled(bool checked)
{
    setFilterStatus(FilterProgrammer::FilterMandelbrot, checked);
}
