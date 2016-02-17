#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fourierfilter.h"
#include "microphonecapturethread.h"
#include "dyplocontext.h"

#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <cstdlib>
#include <ctime>
#include <QDebug>

static DyploContext dyploContext;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    filterProgrammer(dyploContext),
    dyploRouter(dyploContext, filterProgrammer)
{
    ui->setupUi(this);
    ui->partialProgramMetrics->hide();

    ui->node1_overlay->hide();
    ui->node2_overlay->hide();
    ui->node3_overlay->hide();
    ui->node4_overlay->hide();
    ui->node5_overlay->hide();
    ui->node6_overlay->hide();
    ui->node7_overlay->hide();

    demoColorMap[DemoVideo] = QColor(0,0,255);
    demoColorMap[DemoAudio] = QColor(255,0,0);
    demoColorMap[DemoMandelbrot] = QColor(0,255,0);

    connect(&video, SIGNAL(renderedImage(QImage)), ui->video, SLOT(updatePixmap(QImage)));
    connect(&dyploContext, SIGNAL(programmedPartial(int,const char*,uint,uint)), this, SLOT(showProgrammingMetrics(int,const char*,uint,uint)));
}

void MainWindow::programmedDemo(EDemo prDemo, const QList<EPrRegion>& prRegionsUsed)
{
    Q_ASSERT(demoColorMap.contains(prDemo));
    QColor& color = demoColorMap[prDemo];

    demoPrRegionsUsedMap[prDemo] = prRegionsUsed;

    QList<EPrRegion>::const_iterator it;
    for (it = prRegionsUsed.begin(); it != prRegionsUsed.end(); ++it)
    {
        EPrRegion prRegion = *it;

        QLabel* prRegionOverlay = getPrRegion(prRegion);
        Q_ASSERT(prRegionOverlay != NULL);
        prRegionOverlay->setStyleSheet(getOverlayBackgroundColor(color));
        prRegionOverlay->show();
    }
}

void MainWindow::disabledDemo(EDemo prDemo)
{
    Q_ASSERT(demoPrRegionsUsedMap.contains(prDemo));
    QList<EPrRegion>& prRegionsUsed = demoPrRegionsUsedMap[prDemo];
    QList<EPrRegion>::iterator it;
    for (it = prRegionsUsed.begin(); it != prRegionsUsed.end(); ++it)
    {
        EPrRegion prRegion = *it;

        QLabel* prRegionOverlay = getPrRegion(prRegion);
        Q_ASSERT(prRegionOverlay != NULL);
        prRegionOverlay->hide();
    }

    demoPrRegionsUsedMap[prDemo].clear();
}

QLabel* MainWindow::getPrRegion(EPrRegion prRegion)
{
    QLabel* prRegionOverlay = NULL;

    switch (prRegion)
    {
    case PrRegion1:
        prRegionOverlay = ui->node1_overlay;
        break;
    case PrRegion2:
        prRegionOverlay = ui->node2_overlay;
        break;
    case PrRegion3:
        prRegionOverlay = ui->node3_overlay;
        break;
    case PrRegion4:
        prRegionOverlay = ui->node4_overlay;
        break;
    case PrRegion5:
        prRegionOverlay = ui->node5_overlay;
        break;
    case PrRegion6:
        prRegionOverlay = ui->node6_overlay;
        break;
    case PrRegion7:
        prRegionOverlay = ui->node7_overlay;
        break;
    }

    return prRegionOverlay;
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


void MainWindow::hideProgrammingMetrics()
{
    ui->partialProgramMetrics->hide();
}

void MainWindow::showProgrammingMetrics(int node, const char *name, unsigned int size, unsigned int microseconds)
{
    unsigned int mbps = (((unsigned long long)size * 1000000) / microseconds) >> 20;
    ui->partialProgramMetrics->setText(QString("Partial '%1' into %2: %3kB in %4 us (%5 MB/s)").arg(name).arg(node).arg(size>>10).arg(microseconds).arg(mbps));
    ui->partialProgramMetrics->show();

    QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(this);
    ui->partialProgramMetrics->setGraphicsEffect(eff);
    QPropertyAnimation* a = new QPropertyAnimation(eff, "opacity");
    a->setDuration(4000);
    a->setStartValue(1);
    a->setEndValue(0);
    a->setEasingCurve(QEasingCurve::InBack);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    connect(a, SIGNAL(finished()),this, SLOT(hideProgrammingMetrics()), Qt::UniqueConnection);
    connect(a, SIGNAL(finished()),eff, SLOT(deleteLater()), Qt::UniqueConnection);
}

void MainWindow::on_buttonVideodemo_toggled(bool checked)
{
    // TODO: program and route Dyplo
    if (checked)
    {
        if (video.activate(
                    &dyploContext,
                    ui->cbYUVToRGB->isChecked()) != 0)
            updateVideoDemoState(false);
    }
    else
    {
        video.deactivate();
    }
}

void MainWindow::on_buttonAudioDemo_toggled(bool checked)
{
    // TODO: program and route Dyplo
    if (checked)
    {
        bool enableLowPassFIR = ui->cbLowPassFIR->isChecked();
        bool enableHighPassFIR = ui->cbHighPassFIR->isChecked();
        bool enableFFT = ui->cbFFT->isChecked();

    }

    ui->cbLowPassFIR->setEnabled(!checked);
    ui->cbHighPassFIR->setEnabled(!checked);
    ui->cbFFT->setEnabled(!checked);
}

void MainWindow::on_buttonMandelbrotDemo_toggled(bool checked)
{
    // TODO: program and route Dyplo
    if (checked)
    {

    }
}

void MainWindow::updateVideoDemoState(bool active)
{
    ui->buttonVideodemo->setChecked(active);
    ui->cbYUVToRGB->setEnabled(!active);
    ui->cbLaplacian->setEnabled(!active);
}
