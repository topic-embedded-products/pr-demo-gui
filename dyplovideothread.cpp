#include "dyplovideothread.h"
#include "videowidget.h"

#include <QImage>
#include <QMutexLocker>
#include <QDebug>

#include "dyplo/hardware.hpp"

DyploVideoThread::DyploVideoThread(QObject *parent)
    : QThread(parent)
{
    abort = false;

    start(NormalPriority);
}

DyploVideoThread::~DyploVideoThread()
{
    {
        QMutexLocker lock(&mutex);
        abort = true;
    }

    wait();
}

void DyploVideoThread::grabNextFrame()
{
    QMutexLocker locker(&mutex);
    continueGrabbing = true;
    condition.wakeOne();
}

void DyploVideoThread::run()
{
    // Create objects for hardware control
    try
    {
        // TODO implement the Dyplo part:
        dyplo::HardwareContext hardware;
        dyplo::HardwareControl hwControl(hardware);
        dyplo::HardwareDMAFifo mandelbrotNode(hardware.openDMA(0, O_RDONLY));

        static const unsigned int bytes_per_block = VideoWidget::RESOLUTION_X * VideoWidget::RESOLUTION_Y * VideoWidget::BYTES_PER_PIXEL;
        static const unsigned int num_blocks = 2;
        mandelbrotNode.reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, bytes_per_block, num_blocks, true);

        forever {
            dyplo::HardwareDMAFifo::Block* block = mandelbrotNode.dequeue();
            const uchar* pixelbuffer = (const uchar*)block->data;

            if (abort)
            {
                return;
            }

            QImage image(pixelbuffer, VideoWidget::RESOLUTION_X, VideoWidget::RESOLUTION_Y, QImage::Format_RGB888);
            emit renderedImage(image);

            mutex.lock();
            if (!continueGrabbing)
                condition.wait(&mutex);
            continueGrabbing = false;
            mutex.unlock();
        }
    }
    catch (const std::exception& ex)
    {
        qCritical() << "ERROR:\n" << QString(ex.what());
    }
}
