#include "dyplovideothread.h"
#include "videowidget.h"

#include <QImage>
#include <QMutexLocker>
#include <QDebug>

#include "dyplo/hardware.hpp"
#include "dyplocontext.h"

DyploVideoThread::DyploVideoThread(QObject *parent)
    : QThread(parent),
      dyploOutputNodeId(-1)
{
    abort = false;
}

DyploVideoThread::~DyploVideoThread()
{
    stopRendering();
}

void DyploVideoThread::grabNextFrame()
{
    QMutexLocker locker(&mutex);
    continueGrabbing = true;
    condition.wakeOne();
}

void DyploVideoThread::startRendering(int dyploOutputNodeId)
{
    this->dyploOutputNodeId = dyploOutputNodeId;
    abort = false;
    start(NormalPriority);
}

void DyploVideoThread::stopRendering()
{
    {
        QMutexLocker lock(&mutex);
        abort = true;
    }

    wait();
}

void DyploVideoThread::run()
{
    Q_ASSERT(dyploOutputNodeId != -1);

    // Create objects for hardware control
    try
    {
        // TODO test & implement more

        dyplo::HardwareDMAFifo videoOutputNode(DyploContext::getInstance().GetHardwareContext().openDMA(dyploOutputNodeId, O_RDONLY));

        static const unsigned int bytes_per_block = VideoWidget::RESOLUTION_X * VideoWidget::RESOLUTION_Y * VideoWidget::BYTES_PER_PIXEL;
        static const unsigned int num_blocks = 2;
        videoOutputNode.reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, bytes_per_block, num_blocks, true);

        forever {
            /* TODO: enable when integrating
            dyplo::HardwareDMAFifo::Block* block = videoOutputNode.dequeue();
            const uchar* pixelbuffer = (const uchar*)block->data;

            if (abort)
            {
                return;
            }

            // TODO, add scaling
            QImage image(pixelbuffer, VideoWidget::RESOLUTION_X, VideoWidget::RESOLUTION_Y, QImage::Format_RGB888);
            emit renderedImage(image);
            */

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