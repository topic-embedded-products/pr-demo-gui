#include <QPainter>
#include "videowidget.h"

const quint32 VideoWidget::RESOLUTION_X = 421;
const quint32 VideoWidget::RESOLUTION_Y = 291;
const quint32 VideoWidget::BYTES_PER_PIXEL = 3;

VideoWidget::VideoWidget(QWidget *parent) :
    QWidget(parent),
    frameRenderCount(0)
{
    framerateTimer = new QTimer(this);
    connect(framerateTimer, SIGNAL(timeout()), this, SLOT(updateFramerate()));
    framerateTimer->start(1000);
    pixmapOffset = QPoint();
}

VideoWidget::~VideoWidget()
{
    delete framerateTimer;
}

void VideoWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.drawPixmap(pixmapOffset, pixmap);
}

void VideoWidget::updatePixmap(const QImage& image)
{
    frameRenderCount = frameRenderCount + 1;

    pixmap = QPixmap::fromImage(image);
    update();
}

void VideoWidget::updateFramerate()
{
    quint32 fps = frameRenderCount;
    emit frameRate(fps);
    emit trafficFPGA((fps * RESOLUTION_X * RESOLUTION_Y * BYTES_PER_PIXEL) / 1024 / 1024);
    frameRenderCount = 0;
}
