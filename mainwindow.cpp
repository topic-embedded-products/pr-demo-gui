#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dyplocontext.h"
#include "sysfile.hpp"
#include "qprregionlabel.h"

#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <cstdlib>
#include <ctime>
#include <QDebug>

#include "ui_videoframe.h"
#include "ui_fractalframe.h"
#include "ui_floorplanframe.h"
#include "ui_topframe.h"

static DyploContext dyploContext;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    ui_fractal(new Ui::FractalFrame),
    ui_video(new Ui::VideoFrame),
    ui_floorplan(new Ui::FloorplanFrame),
    ui_toppanel(new Ui::TopPanel),
    updateStatsRobin(0)
{
    ui->setupUi(this);

    fractalWidget = new QWidget();
    ui_fractal->setupUi(fractalWidget);

    videoWidget = new QFrame();
    ui_video->setupUi(videoWidget);

    toppanelWidget = new QWidget();
    ui_toppanel->setupUi(toppanelWidget);

    floorplanWidget = new QScrollArea();
    ui_floorplan->setupUi(floorplanWidget);

    QPixmap floorplanImage("/usr/share/floorplan.png");
    if (floorplanImage.isNull())
    {
        qWarning() << "/usr/share/floorplan.png not found, using default 7030 image";
    }
    else
    {
        ui_floorplan->floorplan->setPixmap(floorplanImage);
    }

    parseDyploConfig(&dyploContext.nodeInfo);
    QWidget* nodeParent = ui_floorplan->fFloorplanFPGAContents;
    for(auto& item : dyploContext.nodeInfo)
    {
        QLabel *uiItem;
        switch (item.type)
        {
        case DyploNodeInfo::CPU:
                uiItem = new QLabel("CPU", nodeParent);
                uiItem->setStyleSheet("background-color: rgba(12, 242, 46, 20%);\ncolor: rgb(192, 242, 242);\n");
                break;
        case DyploNodeInfo::DMA:
                uiItem = new QLabel("DMA", nodeParent);
                uiItem->setStyleSheet("background-color: rgba(12, 242, 46, 20%);\ncolor: rgb(192, 242, 242);\n");
                break;
        case DyploNodeInfo::FIXED:
                uiItem = new QLabel(QString::fromStdString(item.function), nodeParent);
                uiItem->setStyleSheet("background-color: rgba(12, 242, 46, 35%);\ncolor: rgb(192, 242, 192);\n");
                break;
        case DyploNodeInfo::PR:
                uiItem = new QPRRegionLabel(nodeParent);
                uiItem->setText("(PR)");
                uiItem->setStyleSheet("background-color: rgba(12, 242, 46, 50%);\ncolor: rgb(192, 242, 192);\n");
                connect(uiItem, SIGNAL(linkActivated(QString)), this, SLOT(prNodeLinkActivated(QString)));
                break;
        default:
            continue; /* Skip unknowns */
        }
        item.widget = uiItem;
        uiItem->setObjectName(QString::number(item.id));
        uiItem->setAlignment(Qt::AlignCenter);
        uiItem->setGeometry(item.geometry);
    }

    ui_fractal->partialProgramMetrics->hide();

    try {
        tempSensor = new IIOTempSensor();
    } catch (const std::exception&) {
        tempSensor = NULL;
        ui_toppanel->lblTemperature->setText("n.a.");
        ui_toppanel->lblTemperature->setEnabled(false);
    }
    try {
        currentSensor = new SupplyCurrentSensor();
        ui_toppanel->lblCurrentCpu->setNum(currentSensor->read_cpu_supply_current_mA());
    } catch (const std::exception&) {
        currentSensor = NULL;
        ui_toppanel->lblCurrentCpu->setText("n.a.");
        ui_toppanel->lblCurrentCpu->setEnabled(false);
        ui_toppanel->lblCurrentFpga->setText("n.a.");
        ui_toppanel->lblCurrentFpga->setEnabled(false);
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
    delete ui_toppanel;
    delete ui_floorplan;
}

/* Display everything in the given rectangle (desktop area) */
void MainWindow::showIn(const QRect &rec)
{
    QVBoxLayout *vla = new QVBoxLayout();
    vla->setSpacing(4);
    vla->setContentsMargins(0, 0, 0, 0);
    vla->addWidget(toppanelWidget);
    centralWidget()->setLayout(vla);
    centralWidget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (rec.width() > 1600 && rec.height() > 900)
    {
        /* Enough space to fit it all in a single window */
        QHBoxLayout *hlb = new QHBoxLayout();
        hlb->setSpacing(4);
        hlb->setContentsMargins(4, 4, 4, 4);
        vla->addLayout(hlb, 1);

        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(4);
        vl->setContentsMargins(0, 0, 0, 0);

        vl->addWidget(videoWidget, 1);
        vl->addWidget(fractalWidget, 1);

        hlb->addLayout(vl, 1);
        hlb->addWidget(floorplanWidget, 1);

        setGeometry(rec);
    }
    else
    {
        vla->addWidget(floorplanWidget, 1);

        setGeometry(rec.right() - width(), rec.top(),
                    width(), rec.height());

        QMainWindow *videoWindow = new QMainWindow(this);
        videoWindow->setCentralWidget(videoWidget);
        /* Cannot get "preferred" to work, so set the minimum and thus make the window large enough */
        ui_video->video->setMinimumHeight(480);
        videoWidget->setParent(videoWindow);
        videoWindow->setWindowTitle("Video (Partial Reconfiguration Demo)");
        videoWindow->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Window);
        videoWindow->setGeometry(rec.left(), rec.top(), videoWindow->width(), videoWindow->height());
        videoWindow->show();

        QMainWindow *fractalWindow = new QMainWindow(this);
        fractalWindow->setCentralWidget(fractalWidget);
        ui_fractal->mandelbrot->setMinimumHeight(480);
        fractalWidget->setParent(fractalWindow);
        fractalWindow->setWindowTitle("Fractal (Partial Reconfiguration Demo)");
        fractalWindow->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Window);
        fractalWindow->setGeometry(rec.left(), rec.bottom() - fractalWindow->height(), fractalWindow->width(), fractalWindow->height());
        fractalWindow->show();
    }
    show();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    bool isSmall = size().width() < 1600 || size().height() < 900;
    ui_toppanel->lblPuzzlePieces->setVisible(!isSmall);
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
    if (!label)
        return;

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
    for (const auto& item: dyploContext.nodeInfo)
    {
        if (item.id == id)
            return item.widget;
    }

    return NULL;
}

void MainWindow::updateFloorplan()
{
    for (const auto& item: dyploContext.nodeInfo)
        hideLabel(item.widget);

    DyploNodeResourceList nodes;
    nodes.clear();
    video.enumDyploResources(nodes);
    for (DyploNodeResourceList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QLabel* l = getPrRegion(it->id);
        if (l) {
            showLabelColor(l, Qt::blue);
            l->setText(it->name);
        }
    }

    nodes.clear();
    mandelbrot.enumDyploResources(nodes);
    for (DyploNodeResourceList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QLabel* l = getPrRegion(it->id);
        if (l) {
            showLabelColor(l, Qt::green);
            l->setText(it->name);
        }
    }

    nodes.clear();
    externals.enumDyploResources(nodes);
    for (DyploNodeResourceList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
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
                    ui_toppanel->lblCPU->setText("---");
                else
                    ui_toppanel->lblCPU->setText(QString("%1%").arg(usage));
        }
        break;
    case 1:
        if (tempSensor)
        {
            try {
                int t = tempSensor->getTempMilliDegrees() / 1000;
                ui_toppanel->lblTemperature->setText(QString("%1 \xB0" "C").arg(t));
            } catch (const std::exception& ex) {
                qDebug() << "Failed reading temperature:" << ex.what();
            }
        }
        if (currentSensor)
        {
            try {
                ui_toppanel->lblCurrentCpu->setText(QString("%1 mW").arg(
                        currentSensor->read_cpu_supply_current_mA()));
                ui_toppanel->lblCurrentFpga->setText(QString("%1 mW").arg(
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

void MainWindow::prNodeLinkActivated(const QString &link)
{
    int node = link.toInt();
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

