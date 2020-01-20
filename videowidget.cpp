#include <QPainter>
#include "videowidget.h"

VideoWidget::VideoWidget(QWidget *parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
}

void VideoWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    int w = width();
    int h = height();
    int pw = pixmap.width();
    int ph = pixmap.height();

    painter.drawPixmap(0, 0, pixmap);
    /* Paint the areas the pixmap did not cover in black */
    painter.fillRect(pw, 0, w - pw, ph, Qt::black);
    painter.fillRect(0, ph, w, h - ph, Qt::black);
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    emit clicked(event);
}

void VideoWidget::updatePixmap(const QImage& image)
{
    framerateCounter.frame();
    pixmap = QPixmap::fromImage(image);
    update();
}
