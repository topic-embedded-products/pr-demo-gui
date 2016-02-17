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
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->partialProgramMetrics->hide();

    connect(&video, SIGNAL(renderedImage(QImage)), ui->video, SLOT(updatePixmap(QImage)));
    connect(&video, SIGNAL(setActive(bool)), this, SLOT(updateVideoDemoState(bool)));
    connect(&dyploContext, SIGNAL(programmedPartial(int,const char*,uint,uint)), this, SLOT(showProgrammingMetrics(int,const char*,uint,uint)));
    connect(&ui->video->framerateCounter, SIGNAL(frameRate(uint,uint)), this, SLOT(showVideoStats(uint,uint)));

    updateFloorplan();
}

MainWindow::~MainWindow()
{
    delete ui;
}

static QString getOverlayBackgroundColor(const QColor& color)
{
    int red, green, blue = 0;
    color.getRgb(&red, &green, &blue);

    return QString("background-color: rgba(%1, %2, %3, 50\%);").arg(red).arg(green).arg(blue);
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
    case 2:
        prRegionOverlay = ui->node1_overlay;
        break;
    case 3:
        prRegionOverlay = ui->node2_overlay;
        break;
    case 4:
        prRegionOverlay = ui->node3_overlay;
        break;
    case 5:
        prRegionOverlay = ui->node4_overlay;
        break;
    case 6:
        prRegionOverlay = ui->node5_overlay;
        break;
    case 7:
        prRegionOverlay = ui->node6_overlay;
        break;
    case 8:
        prRegionOverlay = ui->node7_overlay;
        break;
    }

    return prRegionOverlay;
}

void MainWindow::updateFloorplan()
{
    ui->node1_overlay->hide();
    ui->node2_overlay->hide();
    ui->node3_overlay->hide();
    ui->node4_overlay->hide();
    ui->node5_overlay->hide();
    ui->node6_overlay->hide();
    ui->node7_overlay->hide();

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
}


void MainWindow::hideProgrammingMetrics()
{
    ui->partialProgramMetrics->hide();
}

void MainWindow::showProgrammingMetrics(int node, const char *name, unsigned int size, unsigned int microseconds)
{
    unsigned int mbps = (((unsigned long long)size * 1000000) / microseconds) >> 20;
    ui->partialProgramMetrics->setText(QString("Partial '%1' into %2: %3kB in %4.%5 ms (%6 MB/s)").arg(name).arg(node).arg(size>>10).arg(microseconds/1000).arg((microseconds/100) % 10).arg(mbps));
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

void MainWindow::on_buttonVideodemo_toggled(bool checked)
{
    // TODO: program and route Dyplo
    if (checked)
    {
        ui->lblVideoStats->setText("...");
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
    ui->lblVideoStats->setVisible(active);
    updateFloorplan();
}
