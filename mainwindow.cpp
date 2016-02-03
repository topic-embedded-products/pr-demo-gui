#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fourierfilter.h"

#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <cstdlib>
#include <ctime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    filterProgrammer(DyploContext::getInstance()),
    dyploRouter(DyploContext::getInstance(), filterProgrammer)
{
    ui->setupUi(this);
    ui->partialProgramMetrics->hide();

    hideOverlay(PrRegion1);
    hideOverlay(PrRegion2);
    hideOverlay(PrRegion3);
    hideOverlay(PrRegion4);
    hideOverlay(PrRegion5);
    hideOverlay(PrRegion6);
    hideOverlay(PrRegion7);

    filterColorMap[FilterVideoGrayscale] = QColor(0,0,255);
    filterColorMap[FilterVideoLaplacian] = QColor(0,0,255);
    filterColorMap[FilterAudioFFT] = QColor(255,0,0);
    filterColorMap[FilterAudioHighpass] = QColor(255,0,0);
    filterColorMap[FilterAudioLowpass] = QColor(255,0,0);
    filterColorMap[FilterMandelbrot] = QColor(0,255,0);

    ui->mandelbrot->start(dyploRouter.GetDemoOutputNode(DemoMandelbrot));
    ui->video->start(dyploRouter.GetDemoOutputNode(DemoMandelbrot));


    // temporary test code
    /*
    short input_data[2048];
    gen_signal(2048, &input_data[0], 2000.0f * 5, 3000.0f * (6-1)); // Create input

    FourierFilter* filter = new FourierFilter(2048, 100);
    float* spectrumValues = filter->getSpectrum(&input_data[0]);

    std::srand(std::time(0));
    for (int i = 0; i < 100; i++)
    {
        spectrumValues[i] = std::rand();
    }
    spectrumValues[24] = 15359473.0/2;
    spectrumValues[2] = 15359473.0/3;
    spectrumValues[1] = 15359473.0/1.1;
    */

    // For testing purposes:
    ui->spectrum->setSpectrumSize(100);
    spectrumTimer = new QTimer(this);
    connect(spectrumTimer, SIGNAL(timeout()), this, SLOT(updateSpectrum()));
    spectrumTimer->start(33);

    std::srand(std::time(0));
}

void MainWindow::showOverlay(EPrRegion prRegion, const QColor& color)
{
    switch (prRegion)
    {
    case PrRegion1:
        ui->node1_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node1_overlay->show();
        break;
    case PrRegion2:
        ui->node2_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node2_overlay->show();
        break;
    case PrRegion3:
        ui->node3_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node3_overlay->show();
        break;
    case PrRegion4:
        ui->node4_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node4_overlay->show();
        break;
    case PrRegion5:
        ui->node5_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node5_overlay->show();
        break;
    case PrRegion6:
        ui->node6_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node6_overlay->show();
        break;
    case PrRegion7:
        ui->node7_overlay->setStyleSheet(getOverlayBackgroundColor(color));
        ui->node7_overlay->show();
        break;
    }
}

void MainWindow::hideOverlay(EPrRegion prRegion)
{
    switch (prRegion)
    {
    case PrRegion1:
        ui->node1_overlay->hide();
        break;
    case PrRegion2:
        ui->node2_overlay->hide();
        break;
    case PrRegion3:
        ui->node3_overlay->hide();
        break;
    case PrRegion4:
        ui->node4_overlay->hide();
        break;
    case PrRegion5:
        ui->node5_overlay->hide();
        break;
    case PrRegion6:
        ui->node6_overlay->hide();
        break;
    case PrRegion7:
        ui->node7_overlay->hide();
        break;
    }
}

void MainWindow::updateSpectrum()
{
    for (int i = 0; i < 100; i++)
    {
        spectrumValues[i] = std::rand() % 100;
    }

    ui->spectrum->updateSpectrum(spectrumValues);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete spectrumTimer;
}

QString MainWindow::getOverlayBackgroundColor(const QColor& color)
{
    int red, green, blue = 0;
    color.getRgb(&red, &green, &blue);

    return QString("background-color: rgba(%1, %2, %3, 50\%);").arg(red).arg(green).arg(blue);
}

void MainWindow::on_buttonGrayscale_toggled(bool checked)
{
    setFilterStatus(FilterVideoGrayscale, checked);
}

void MainWindow::setFilterStatus(EFilters filter, bool enabled)
{
    if (enabled)
    {
        EPrRegion programRegion;
        quint32 programTimeMs;
        bool programmingSucceeded = filterProgrammer.ProgramFilter(filter, programRegion, programTimeMs);
        if (programmingSucceeded)
        {
            showProgrammingMetrics(programRegion, programTimeMs);
            dyploRouter.RouteFilter(filter);

            // TODO: show performance metrics of programming
            Q_ASSERT(filterColorMap.contains(filter));
            showOverlay(programRegion, filterColorMap[filter]);
        }
    }
    else
    {
        EPrRegion disableRegion;
        dyploRouter.UnrouteFilter(filter);
        bool disabledNode = filterProgrammer.DisableFilter(filter, disableRegion);
        if (disabledNode)
        {
            hideOverlay(disableRegion);
        }
    }
}

void MainWindow::on_buttonLaplacian_toggled(bool checked)
{
    setFilterStatus(FilterVideoLaplacian, checked);
}

void MainWindow::on_buttonLowPass_toggled(bool checked)
{
    setFilterStatus(FilterAudioLowpass, checked);
}

void MainWindow::on_buttonHighPass_toggled(bool checked)
{
    setFilterStatus(FilterAudioHighpass, checked);
}

void MainWindow::on_buttonFFT_toggled(bool checked)
{
    setFilterStatus(FilterAudioFFT, checked);
}

void MainWindow::on_buttonMandelbrot_toggled(bool checked)
{
    setFilterStatus(FilterMandelbrot, checked);
}

void MainWindow::hideProgrammingMetrics()
{
    ui->partialProgramMetrics->hide();
}

void MainWindow::showProgrammingMetrics(EPrRegion programRegion,
                                    quint32 programTimeMs)
{
    ui->partialProgramMetrics->setText(QString("Partial bitstream programmed on PR Node %1 in %2 ms")
        .arg(programRegion).arg(programTimeMs));
    ui->partialProgramMetrics->show();

    QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(this);
    ui->partialProgramMetrics->setGraphicsEffect(eff);
    QPropertyAnimation* a = new QPropertyAnimation(eff, "opacity");
    a->setDuration(2000);
    a->setStartValue(1);
    a->setEndValue(0);
    a->setEasingCurve(QEasingCurve::InBack);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    connect(a, SIGNAL(finished()),this, SLOT(hideProgrammingMetrics()), Qt::UniqueConnection);
    connect(a, SIGNAL(finished()),eff, SLOT(deleteLater()), Qt::UniqueConnection);
}
