#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dyplocontext.h"
#include "sysfile.hpp"

#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <cstdlib>
#include <ctime>
#include <QDebug>

#include "ui_videoframe.h"
#include "ui_fractalframe.h"

/* Nodes in logic 7030:
 * 0 CPU
 * 1 CPU
 * 2 MUX
 * 3 MUX
 * 4..11 PR
 * 12..14 DMA
 * 15 ICAP
 */

/* Nodes in logic 7015:
 * 0 CPU
 * 1..4 PR
 * 5..6 DMA
 * 7 ICAP
 */

static DyploContext dyploContext;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    ui_fractal(new Ui::FractalFrame),
    ui_video(new Ui::VideoFrame),
    updateStatsRobin(0)
{
    ui->setupUi(this);

    fractalWindow = new QMainWindow(this);
    QWidget *ff = new QWidget(fractalWindow);
    fractalWindow->setCentralWidget(ff);
    ui_fractal->setupUi(ff);
    fractalWindow->setWindowTitle("Fractal (Partial Reconfiguration Demo)");
    fractalWindow->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Window);

    videoWindow = new QMainWindow(this);
    QFrame *f = new QFrame(videoWindow);
    videoWindow->setCentralWidget(f);
    ui_video->setupUi(f);
    videoWindow->setWindowTitle("Video (Partial Reconfiguration Demo)");
    videoWindow->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Window);

    ui_fractal->partialProgramMetrics->hide();

    ui->node4_overlay->setObjectName("4");
    ui->node5_overlay->setObjectName("5");
    ui->node6_overlay->setObjectName("6");
    ui->node7_overlay->setObjectName("7");
    ui->node8_overlay->setObjectName("8");
    ui->node9_overlay->setObjectName("9");
    ui->node10_overlay->setObjectName("10");
    ui->node11_overlay->setObjectName("11");

    try {
        tempSensor = new IIOTempSensor();
    } catch (const std::exception&) {
        tempSensor = NULL;
        ui->lblTemperature->setText("n.a.");
        ui->lblTemperature->setEnabled(false);
    }
    try {
        currentSensor = new SupplyCurrentSensor();
        ui->lblCurrentCpu->setNum(currentSensor->read_cpu_supply_current_mA());
    } catch (const std::exception&) {
        currentSensor = NULL;
        ui->lblCurrentCpu->setText("n.a.");
        ui->lblCurrentCpu->setEnabled(false);
        ui->lblCurrentFpga->setText("n.a.");
        ui->lblCurrentFpga->setEnabled(false);
    }

    connect(&cpuStatsTimer, SIGNAL(timeout()), this, SLOT(updateCpuStats()));
    cpuStatsTimer.start(1000);

    connect(&video, SIGNAL(renderedImage(QImage)), ui_video->video, SLOT(updatePixmap(QImage)));
    connect(&video, SIGNAL(setActive(bool)), this, SLOT(updateVideoDemoState(bool)));
    connect(&dyploContext, SIGNAL(programmedPartial(int,const char*,uint,uint)), this, SLOT(showProgrammingMetrics(int,const char*,uint,uint)));
    connect(&ui_video->video->framerateCounter, SIGNAL(frameRate(uint,uint)), this, SLOT(showVideoStats(uint,uint)));

    connect(&mandelbrot, SIGNAL(renderedImage(QImage)), ui_fractal->mandelbrot, SLOT(updatePixmap(QImage)));
    connect(&mandelbrot, SIGNAL(setActive(bool)), this, SLOT(updateMandelbrotDemoState(bool)));
    connect(&ui_fractal->mandelbrot->framerateCounter, SIGNAL(frameRate(uint,uint)), this, SLOT(showMandelbrotStats(uint,uint)));

    connect(ui->node4_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
    connect(ui->node5_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
    connect(ui->node6_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
    connect(ui->node7_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
    connect(ui->node8_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
    connect(ui->node9_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
    connect(ui->node10_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
    connect(ui->node11_overlay, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));

    connect(ui_fractal->mandelbrot, SIGNAL(clicked(QMouseEvent*)), this, SLOT(mandelbrotClicked(QMouseEvent*)));

    connect(ui_video->buttonVideodemo, SIGNAL(toggled(bool)), this, SLOT(buttonVideodemo_toggled(bool)));
    connect(ui_fractal->buttonMandelbrotDemo, SIGNAL(toggled(bool)), this, SLOT(buttonMandelbrotDemo_toggled(bool)));
    connect(ui_fractal->btnPresetA, SIGNAL(pressed()), this, SLOT(btnPresetA_clicked()));
    connect(ui_fractal->btnPresetB, SIGNAL(pressed()), this, SLOT(btnPresetB_clicked()));
    connect(ui_fractal->btnPresetC, SIGNAL(pressed()), this, SLOT(btnPresetC_clicked()));

    updateFloorplan();
}

MainWindow::~MainWindow()
{
    mandelbrot.deactivate(); /* Calls back to UI, so we must do this before destroying the UI */
    video.deactivate();
    delete ui;
    delete ui_fractal;
    delete ui_video;
}

/* Display everything in the given rectangle (desktop area) */
void MainWindow::showIn(const QRect &rec)
{
    setGeometry(rec.right() - width(), rec.top(),
                width(), rec.height());

    videoWindow->setGeometry(rec.left(), rec.top(), videoWindow->width(), videoWindow->height());
    videoWindow->show();

    fractalWindow->setGeometry(rec.left(), rec.bottom() - fractalWindow->height(), fractalWindow->width(), fractalWindow->height());
    fractalWindow->show();

    show();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    bool isSmall = size().width() < 1600;
    ui->lblPuzzlePieces->setVisible(!isSmall);
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

static void hideLabel(QLabel* label)
{
    label->setStyleSheet("background-color: rgba(0,0,0,0%);\n"
                         "color: rgba(0,0,0);");
    label->setText("");
}

static void showLabelColor(QLabel* label, const QColor& color)
{
    label->setStyleSheet(getOverlayBackgroundColor(color));
}

QLabel* MainWindow::getPrRegion(int id)
{
    QLabel* prRegionOverlay = NULL;
    /* Nodes in logic 7030:
     * 0 CPU
     * 1 CPU
     * 2 MUX
     * 3 MUX
     * 4..11 PR
     * 12..14 DMA
     * 15 ICAP
     */

    /* Nodes in logic 7015:
     * 0 CPU
     * 1..4 PR
     * 5..6 DMA
     * 7 ICAP
     */

    /* Rather lame check on 7015 system configuration */
    if (dyploContext.num_dma_nodes < 3)
    {
        switch (id)
        {
        /* CPU node */
        case 0:
            prRegionOverlay = ui->node0_overlay;
            break;
        /* PR nodes */
        case 1:
            prRegionOverlay = ui->node4_overlay;
            break;
        case 2:
            prRegionOverlay = ui->node5_overlay;
            break;
        case 3:
            prRegionOverlay = ui->node6_overlay;
            break;
        case 4:
            prRegionOverlay = ui->node7_overlay;
            break;
        /* DMA nodes */
        case 5:
            prRegionOverlay = ui->node12_overlay;
            break;
        case 6:
            prRegionOverlay = ui->node13_overlay;
            break;
        }
    }
    else
    {
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
        case 14:
            prRegionOverlay = ui->node14_overlay;
            break;
        }
    }

    return prRegionOverlay;
}

void MainWindow::updateFloorplan()
{
    hideLabel(ui->node0_overlay);
    hideLabel(ui->node1_overlay);
    hideLabel(ui->node2_overlay);
    hideLabel(ui->node3_overlay);
    hideLabel(ui->node4_overlay);
    hideLabel(ui->node5_overlay);
    hideLabel(ui->node6_overlay);
    hideLabel(ui->node7_overlay);
    hideLabel(ui->node8_overlay);
    hideLabel(ui->node9_overlay);
    hideLabel(ui->node10_overlay);
    hideLabel(ui->node11_overlay);
    hideLabel(ui->node12_overlay);
    hideLabel(ui->node13_overlay);
    hideLabel(ui->node14_overlay);

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
    ui_fractal->partialProgramMetrics->setText("");
    ui_fractal->partialProgramMetrics->hide();
}

void MainWindow::showProgrammingMetrics(int node, const char *name, unsigned int size, unsigned int microseconds)
{
    unsigned int mbps = (((unsigned long long)size * 1000000) / microseconds) >> 20;
    if (!programmingMetrics.isEmpty())
        programmingMetrics += "\n";
    programmingMetrics += QString("Partial '%1' into %2:\n%3kB in %4.%5 ms (%6 MB/s)").arg(name).arg(node).arg(size>>10).arg(microseconds/1000).arg((microseconds/100) % 10).arg(mbps);
    ui_fractal->partialProgramMetrics->setText(programmingMetrics);
    ui_fractal->partialProgramMetrics->show();

    QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(this);
    ui_fractal->partialProgramMetrics->setGraphicsEffect(eff);
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
    ui_video->lblVideoStats->setText(message);
}

void MainWindow::showMandelbrotStats(unsigned int frames, unsigned int milliseconds)
{
    QString message;
    if (frames)
        message = QString("%1 ms").arg(milliseconds / frames);
    else
        message = "-";
    ui_fractal->lblMandelbrotStats->setText(message);
}

void MainWindow::buttonVideodemo_toggled(bool checked)
{
    if (checked)
    {
        ui_video->lblVideoStats->setText("---");
        if (video.activate(
                    &dyploContext,
                    ui_video->cbYUVToRGB->isChecked(),
                    ui_video->cbFilterContrast->isChecked(),
                    ui_video->cbFilterGray->isChecked(),
                    ui_video->cbFilterTreshold->isChecked()))
            updateVideoDemoState(false);
    }
    else
    {
        video.deactivate();
    }
}

void MainWindow::buttonMandelbrotDemo_toggled(bool checked)
{
    if (checked)
    {
        ui_fractal->lblMandelbrotStats->setText("...");
        if (mandelbrot.activate(&dyploContext, 8))
            updateMandelbrotDemoState(false); /* Failed to init, update UI */
    }
    else
        mandelbrot.deactivate();
}

void MainWindow::mandelbrotClicked(QMouseEvent *event)
{
    double x = mandelbrot.getX();
    double y = mandelbrot.getY();
    double z = mandelbrot.getZ();

    int w2 = ui_fractal->mandelbrot->width() / 2;
    int h2 = ui_fractal->mandelbrot->height() / 2;

    mandelbrot.setCoordinates(
                x + z * (event->x() - w2),
                y + z * (event->y() - h2));
}

void MainWindow::updateCpuStats()
{
    switch (updateStatsRobin)
    {
    case 0:
        {
            int usage = cpuInfo.getCurrentValue();
                if (usage < 0)
                    ui->lblCPU->setText("---");
                else
                    ui->lblCPU->setText(QString("%1%").arg(usage));
        }
        break;
    case 1:
        if (tempSensor)
        {
            try {
                int t = tempSensor->getTempMilliDegrees() / 1000;
                ui->lblTemperature->setText(QString("%1 \xB0" "C").arg(t));
            } catch (const std::exception& ex) {
                qDebug() << "Failed reading temperature:" << ex.what();
            }
        }
        if (currentSensor)
        {
            try {
                ui->lblCurrentCpu->setText(QString("%1 mW").arg(
                        currentSensor->read_cpu_supply_current_mA()));
                ui->lblCurrentFpga->setText(QString("%1 mW").arg(
                        currentSensor->read_fpga_supply_current_mA()));
            } catch (const std::exception& ex) {
                qDebug() << "Failed reading current:" << ex.what();
            }
        }
        break;
    }
    updateStatsRobin = (updateStatsRobin + 1) % 2;
}

void MainWindow::updateVideoDemoState(bool active)
{
    ui_video->buttonVideodemo->setChecked(active);
    ui_video->cbYUVToRGB->setEnabled(!active);
    ui_video->cbFilterContrast->setEnabled(!active);
    ui_video->cbFilterGray->setEnabled(!active);
    ui_video->cbFilterTreshold->setEnabled(!active);
    ui_video->lblVideoStats->setVisible(active);
    updateFloorplan();
}

void MainWindow::updateMandelbrotDemoState(bool active)
{
    ui_fractal->buttonMandelbrotDemo->setChecked(active);
    ui_fractal->lblMandelbrotStats->setVisible(active);
    updateFloorplan();
}

static int gui_id_to_node(int id)
{
    /* Rather lame check on 7015 vs 7030 system configuration */
    if (dyploContext.num_dma_nodes > 2)
        return id;

    switch (id)
    {
    case 0:
        return 0;
    /* PR nodes */
    case 4:
        return 1;
    case 5:
        return 2;
    case 6:
        return 3;
    case 7:
        return 4;
    /* DMA nodes */
    case 12:
        return 5;
    case 13:
        return 6;
    }

    return -1;
}

void MainWindow::prNodeLinkActivated(const QString &link)
{
    int node = gui_id_to_node(link.toInt());
    if (node < 0)
        return;
    if (externals.isAquired(node))
    {
        externals.release(node);
    }
    else
    {
        if (!externals.aquire(&dyploContext, node))
            return;
    }
    updateFloorplan();
}

void MainWindow::btnPresetA_clicked()
{
    mandelbrot.setCoordinates(-0.86122562296399741, -0.23139131123653386);
    mandelbrot.resetZoom();
}

void MainWindow::btnPresetB_clicked()
{
    mandelbrot.setCoordinates(-1.1623415998834443208, -0.29236893389210100169);
    mandelbrot.resetZoom();
}

void MainWindow::btnPresetC_clicked()
{
    mandelbrot.setCoordinates(-1.017809644426762361, 0.28358540656703479232);
    mandelbrot.resetZoom();
}

