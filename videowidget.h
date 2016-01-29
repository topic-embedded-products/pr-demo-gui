#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QPixmap>
#include <QWidget>
#include <QTimer>
#include "dyplovideothread.h"

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent = 0);
    ~VideoWidget();

    static const quint32 RESOLUTION_X;
    static const quint32 RESOLUTION_Y;
    static const quint32 BYTES_PER_PIXEL;

public slots:
    void start(int dyploNodeId);
    void stop();

protected:
    virtual void paintEvent(QPaintEvent *event);

signals:
    void frameRate(const quint32 framerate);
    void trafficFPGA(const quint32 megaByteSec);

private slots:
    void updatePixmap(const QImage &image);
    void updateFramerate();

private:
    DyploVideoThread dyplothread;
    QAtomicInt frameRenderCount;
    QTimer* framerateTimer;
    QPixmap pixmap;
    QPoint pixmapOffset;


};

#endif
