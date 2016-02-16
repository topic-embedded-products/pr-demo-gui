#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QPixmap>
#include <QWidget>
#include "frameratecounter.h"

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent = 0);

protected:
    virtual void paintEvent(QPaintEvent *event);

public slots:
    void updatePixmap(const QImage &image);

public:
    FrameRateCounter framerateCounter;

private:
    QPixmap pixmap;
};

#endif
