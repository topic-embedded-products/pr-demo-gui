#ifndef VIDEOPIPELINE_H
#define VIDEOPIPELINE_H

#include <QObject>
#include "video-capture.h"

class QImage;
class QSocketNotifier;

class VideoPipeline: public QObject
{
    Q_OBJECT

public:
    VideoPipeline();
    ~VideoPipeline();

    int activate();
    void deactivate();

signals:
    void renderedImage(const QImage &image);
    void setActive(bool active);

private slots:
    void frameAvailable(int socket);

protected:
    VideoCapture capture;
    QSocketNotifier* socketNotifier;
    unsigned char* rgb_buffer;
};

#endif // VIDEOPIPELINE_H
