#include "dyplomandelbrotthread.h"
#include "fpgamandelbrotwidget.h"

#include <QImage>
#include <QMutexLocker>
#include <QDebug>

#include "dyplo/hardware.hpp"

DyploMandelbrotThread::DyploMandelbrotThread(QObject* parent)
    : QThread(parent),
      dyploOutputNodeId(-1)
{
    abort = false;
}

DyploMandelbrotThread::~DyploMandelbrotThread()
{
    stopRendering();
}

void DyploMandelbrotThread::grabNextFrame()
{
    QMutexLocker locker(&mutex);
    continueGrabbing = true;
    condition.wakeOne();
}

void DyploMandelbrotThread::startRendering(int dyploOutputNodeId)
{
    this->dyploOutputNodeId = dyploOutputNodeId;
    abort = false;
    start(NormalPriority);
}

void DyploMandelbrotThread::stopRendering()
{
    {
        QMutexLocker lock(&mutex);
        abort = true;
    }

    wait();
}

void DyploMandelbrotThread::run()
{
    Q_ASSERT(dyploOutputNodeId != -1);

    // Create objects for hardware control
    try
    {
        // TODO test & implement more
        dyplo::HardwareDMAFifo mandelbrotOutput(DyploContext::getInstance().GetHardwareContext().openDMA(dyploOutputNodeId, O_RDONLY));

        static const unsigned int bytes_per_block = FpgaMandelbrotWidget::RESOLUTION_X * FpgaMandelbrotWidget::RESOLUTION_Y * FpgaMandelbrotWidget::BYTES_PER_PIXEL;
        static const unsigned int num_blocks = 2;
        mandelbrotOutput.reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, bytes_per_block, num_blocks, true);

        forever {
            /* TODO: enable when integrating
            dyplo::HardwareDMAFifo::Block* block = mandelbrotOutput.dequeue();
            const uchar* pixelbuffer = (const uchar*)block->data;

            if (abort)
            {
                return;
            }

            QImage image(pixelbuffer, FpgaMandelbrotWidget::RESOLUTION_X, FpgaMandelbrotWidget::RESOLUTION_Y, QImage::Format_RGB888);
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
