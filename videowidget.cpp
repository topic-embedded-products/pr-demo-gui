#include <QPainter>
#include "videowidget.h"

VideoWidget::VideoWidget(QWidget *parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_PaintOnScreen);
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
    int w = width();
    int h = height();
    if (image.width() <= w && image.height() <= h)
    {
        pixmap = QPixmap::fromImage(image);
        pixmap.detach(); /* Make sure we're not sharing the data */
    }
    else
    {
        QRect r;
        if (w > image.width()) {
            r.setLeft(0);
            r.setWidth(image.width());
        } else {
            r.setLeft((image.width() - w) >> 1);
            r.setWidth(w);
        }
        if (h > image.height()) {
            r.setTop(0);
            r.setHeight(image.height());
        } else {
            r.setTop((image.height() - h) >> 1);
            r.setHeight(h);
        }
        pixmap = QPixmap::fromImage(image.copy(r));
    }
    update();
}
