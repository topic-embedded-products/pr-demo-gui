#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dyplocontext.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <cstdlib>
#include <ctime>
#include <QDebug>

/* Nodes in logic:
 * 0 CPU
 * 1 CPU
 * 2 MUX
 * 3 MUX
 * 4..10 PR
 * 11..13 DMA
 * 14 ICAP
 */

static DyploContext dyploContext;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->partialProgramMetrics->hide();

    connect(&cpuStatsTimer, SIGNAL(timeout()), this, SLOT(updateCpuStats()));
    cpuStatsTimer.start(2000);

    connect(&video, SIGNAL(renderedImage(QImage)), ui->video, SLOT(updatePixmap(QImage)));
    connect(&video, SIGNAL(setActive(bool)), this, SLOT(updateVideoDemoState(bool)));
    connect(&dyploContext, SIGNAL(programmedPartial(int,const char*,uint,uint)), this, SLOT(showProgrammingMetrics(int,const char*,uint,uint)));
    connect(&ui->video->framerateCounter, SIGNAL(frameRate(uint,uint)), this, SLOT(showVideoStats(uint,uint)));

    connect(&mandelbrot, SIGNAL(renderedImage(QImage)), ui->mandelbrot, SLOT(updatePixmap(QImage)));
    connect(&mandelbrot, SIGNAL(setActive(bool)), this, SLOT(updateMandelbrotDemoState(bool)));
    connect(&ui->mandelbrot->framerateCounter, SIGNAL(frameRate(uint,uint)), this, SLOT(showMandelbrotStats(uint,uint)));

    updateFloorplan();
}

MainWindow::~MainWindow()
{
    mandelbrot.deactivate(); /* Calls back to UI, so we must do this before destroying the UI */
    video.deactivate();
    delete ui;
}

static int bright(int c)
{
    if (c >= 128)
        return 255;
    return c + 128;
}

static QString getOverlayBackgroundColor(const QColor& color)
{
    int red, green, blue = 0;
    color.getRgb(&red, &green, &blue);

    return QString("background-color: rgba(%1,%2,%3,50\%);\n"
                   "color: rgb(%4,%5,%6);")
            .arg(red).arg(green).arg(blue)
            .arg(bright(red)).arg(bright(green)).arg(bright(blue));
}

static void showLabelColor(QLabel* label, const QColor& color)
{
    label->setStyleSheet(getOverlayBackgroundColor(color));
    label->show();
}

QLabel* MainWindow::getPrRegion(int id)
{
    QLabel* prRegionOverlay = NULL;

    switch (id)
    {
    case 0:
        prRegionOverlay = ui->node0_overlay;
        break;
    case 1:
        prRegionOverlay = ui->node1_overlay;
        break;
    case 2:
        prRegionOverlay = ui->node2_overlay;
        break;
    case 3:
        prRegionOverlay = ui->node3_overlay;
        break;
    case 4:
        prRegionOverlay = ui->node4_overlay;
        break;
    case 5:
        prRegionOverlay = ui->node5_overlay;
        break;
    case 6:
        prRegionOverlay = ui->node6_overlay;
        break;
    case 7:
        prRegionOverlay = ui->node7_overlay;
        break;
    case 8:
        prRegionOverlay = ui->node8_overlay;
        break;
    case 9:
        prRegionOverlay = ui->node9_overlay;
        break;
    case 10:
        prRegionOverlay = ui->node10_overlay;
        break;
    case 11:
        prRegionOverlay = ui->node11_overlay;
        break;
    case 12:
        prRegionOverlay = ui->node12_overlay;
        break;
    case 13:
        prRegionOverlay = ui->node13_overlay;
        break;
    }

    return prRegionOverlay;
}

void MainWindow::updateFloorplan()
{
    ui->node0_overlay->hide();
    ui->node1_overlay->hide();
    ui->node2_overlay->hide();
    ui->node3_overlay->hide();
    ui->node4_overlay->hide();
    ui->node5_overlay->hide();
    ui->node6_overlay->hide();
    ui->node7_overlay->hide();
    ui->node8_overlay->hide();
    ui->node9_overlay->hide();
    ui->node10_overlay->hide();
    ui->node11_overlay->hide();
    ui->node12_overlay->hide();
    ui->node13_overlay->hide();

    DyploNodeInfoList nodes;
    nodes.clear();
    video.enumDyploResources(nodes);
    for (DyploNodeInfoList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QLabel* l = getPrRegion(it->id);
        if (l) {
            showLabelColor(l, Qt::blue);
            l->setText(it->name);
        }
    }

    nodes.clear();
    mandelbrot.enumDyploResources(nodes);
    for (DyploNodeInfoList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QLabel* l = getPrRegion(it->id);
        if (l) {
            showLabelColor(l, Qt::green);
            l->setText(it->name);
        }
    }

    nodes.clear();
    externals.enumDyploResources(nodes);
    for (DyploNodeInfoList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QLabel* l = getPrRegion(it->id);
        if (l) {
            showLabelColor(l, Qt::gray);
            l->setText(it->name);
        }
    }
}

void MainWindow::externalResourceEnable(int id, bool active)
{
    if (active)
    {
        externals.aquire(&dyploContext, id);
    }
    else
    {
        externals.release(id);
    }
    updateFloorplan();
}


void MainWindow::hideProgrammingMetrics()
{
    programmingMetrics.clear();
    ui->partialProgramMetrics->setText("");
    ui->partialProgramMetrics->hide();
}

void MainWindow::showProgrammingMetrics(int node, const char *name, unsigned int size, unsigned int microseconds)
{
    unsigned int mbps = (((unsigned long long)size * 1000000) / microseconds) >> 20;
    if (!programmingMetrics.isEmpty())
        programmingMetrics += "\n";
    programmingMetrics += QString("Partial '%1' into %2:\n%3kB in %4.%5 ms (%6 MB/s)").arg(name).arg(node).arg(size>>10).arg(microseconds/1000).arg((microseconds/100) % 10).arg(mbps);
    ui->partialProgramMetrics->setText(programmingMetrics);
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

void MainWindow::showVideoStats(unsigned int frames, unsigned int milliseconds)
{
    QString message;
    if (frames)
        message = QString("%1 FPS").arg(((frames*1000)+500)/milliseconds);
    else
        message = "-";
    ui->lblVideoStats->setText(message);
}

void MainWindow::showMandelbrotStats(unsigned int frames, unsigned int milliseconds)
{
    QString message;
    if (frames)
        message = QString("%1 ms").arg(milliseconds / frames);
    else
        message = "-";
    ui->lblMandelbrotStats->setText(message);
}

void MainWindow::on_buttonVideodemo_toggled(bool checked)
{
    if (checked)
    {
        ui->lblVideoStats->setText("...");
        if (video.activate(
                    &dyploContext,
                    ui->cbYUVToRGB->isChecked(),
                    ui->cbFilterContrast->isChecked(),
                    ui->cbFilterGray->isChecked(),
                    ui->cbFilterTreshold->isChecked()))
            updateVideoDemoState(false);
    }
    else
    {
        video.deactivate();
    }
}

void MainWindow::on_buttonMandelbrotDemo_toggled(bool checked)
{
    if (checked)
    {
        ui->lblMandelbrotStats->setText("...");
        mandelbrot.activate(&dyploContext);
    }
    else
        mandelbrot.deactivate();
}

void MainWindow::updateCpuStats()
{
    int usage = cpuInfo.getCurrentValue();
    if (usage < 0)
        ui->lblCPU->setText("---");
    else
        ui->lblCPU->setText(QString("%1%").arg(usage));
}

void MainWindow::updateVideoDemoState(bool active)
{
    ui->buttonVideodemo->setChecked(active);
    ui->cbYUVToRGB->setEnabled(!active);
    ui->cbFilterContrast->setEnabled(!active);
    ui->cbFilterGray->setEnabled(!active);
    ui->cbFilterTreshold->setEnabled(!active);
    ui->lblVideoStats->setVisible(active);
    updateFloorplan();
}

void MainWindow::updateMandelbrotDemoState(bool active)
{
    ui->buttonMandelbrotDemo->setChecked(active);
    ui->lblMandelbrotStats->setVisible(active);
    updateFloorplan();
}
