#ifndef FPGAMANDELBROTWIDGET_H
#define FPGAMANDELBROTWIDGET_H

#include <QPixmap>
#include <QWidget>
#include <QTimer>
#include "dyplomandelbrotthread.h"

class FpgaMandelbrotWidget : public QWidget
{
    Q_OBJECT

public:
    FpgaMandelbrotWidget(QWidget *parent = 0);
    ~FpgaMandelbrotWidget();

    static const quint32 RESOLUTION_X;
    static const quint32 RESOLUTION_Y;
    static const quint32 BYTES_PER_PIXEL;

protected:
    virtual void paintEvent(QPaintEvent *event);

signals:
    void frameRate(const quint32 framerate);
    void trafficFPGA(const quint32 megaByteSec);

private slots:
    void updatePixmap(const QImage &image);
    void updateFramerate();

private:
    DyploMandelbrotThread dyplothread;
    QAtomicInt frameRenderCount;
    QTimer* framerateTimer;
    QPixmap pixmap;
    QPoint pixmapOffset;


};

#endif
