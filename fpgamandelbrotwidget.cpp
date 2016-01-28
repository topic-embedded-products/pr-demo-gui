#include <QPainter>
#include "fpgamandelbrotwidget.h"

const quint32 FpgaMandelbrotWidget::RESOLUTION_X = 421;
const quint32 FpgaMandelbrotWidget::RESOLUTION_Y = 291;
const quint32 FpgaMandelbrotWidget::BYTES_PER_PIXEL = 3;

FpgaMandelbrotWidget::FpgaMandelbrotWidget(QWidget *parent) :
    QWidget(parent),
    dyplothread(),
    frameRenderCount(0)
{
    connect(&dyplothread, SIGNAL(renderedImage(QImage)), this, SLOT(updatePixmap(QImage)));

    framerateTimer = new QTimer(this);
    connect(framerateTimer, SIGNAL(timeout()), this, SLOT(updateFramerate()));
    framerateTimer->start(1000);
    pixmapOffset = QPoint();

    setWindowTitle("Topic Products - Mandelbrot demo");
}

FpgaMandelbrotWidget::~FpgaMandelbrotWidget()
{
    delete framerateTimer;
}

void FpgaMandelbrotWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.drawPixmap(pixmapOffset, pixmap);
}

void FpgaMandelbrotWidget::updatePixmap(const QImage& image)
{
    frameRenderCount = frameRenderCount + 1;

    pixmap = QPixmap::fromImage(image);
    update();
    dyplothread.grabNextFrame();
}

void FpgaMandelbrotWidget::updateFramerate()
{
    quint32 fps = frameRenderCount;
    emit frameRate(fps);
    emit trafficFPGA((fps * RESOLUTION_X * RESOLUTION_Y * BYTES_PER_PIXEL) / 1024 / 1024);
    frameRenderCount = 0;
}
