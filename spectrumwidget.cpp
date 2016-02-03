#include "spectrumwidget.h"

#include <QPainter>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <QTime>
#include <cmath>
#include <QEvent>

const unsigned int SpectrumWidget::SPECTRUM_SIZE = 100;

SpectrumWidget::SpectrumWidget(QWidget *parent) :
    QWidget(parent),
    iSpectrumSize(SPECTRUM_SIZE),
    iMaxValue(0.0f),
    iNewSpectrum(NULL),
    iDrawnCurrentBars(false),
    iCurrentBarHeight(NULL),
    iMicrophoneCapture(),
    iFourierFilter(MicrophoneCaptureThread::CAPTURE_SIZE, iSpectrumSize)
{
    iCurrentBarHeight = new int[iSpectrumSize];
    memset(iCurrentBarHeight, 0, iSpectrumSize*sizeof(int));

    // force no complete redraw
    setAttribute(Qt::WA_OpaquePaintEvent);

    connect(&iMicrophoneCapture, SIGNAL(capturedAudio(short*,uint)), this, SLOT(audioData(short*,uint)));
    QString audioDeviceName("default");
    iMicrophoneCapture.startCapturing(audioDeviceName);

    //this->installEventFilter(this);
}

SpectrumWidget::~SpectrumWidget()
{
    delete[] iCurrentBarHeight;
}

/*
bool SpectrumWidget::eventFilter(QObject* object, QEvent* event)
{
    if (object != this)
    {
        return true;
    }
    return false;
}
*/

void SpectrumWidget::paintEvent(QPaintEvent* )
{
    QPainter painter(this);

    if (iNewSpectrum != NULL)
    {
        float widthPerBar = (frameSize().width()/(float)iSpectrumSize) - 1.0;
        Q_ASSERT(widthPerBar > 0.0f);

        double intPart;
        double fractionalPart = modf(widthPerBar, &intPart);

        widthPerBar = floor(widthPerBar);
        int maxHeightPerBar = frameSize().height() * 0.95;

        int frameHeight = frameSize().height();

        int drawOffsetX = 1.0;
        double fractionalPartCounter = 0.0f;

        for (int i = 0; i < iSpectrumSize; ++i)
        {
            fractionalPartCounter += fractionalPart;

            float barHeightFactor = iNewSpectrum[i] / iMaxValue;
            int barHeight = maxHeightPerBar * barHeightFactor;
            float barWidth = widthPerBar;
            if (fractionalPartCounter > 1.0f)
            {
                fractionalPartCounter -= 1.0f;
                barWidth += 1.0f;
            }

            if (iCurrentBarHeight[i] == 0)
            {
                // initial drawing
                painter.fillRect(drawOffsetX, frameHeight-barHeight, barWidth, barHeight, Qt::white);
            }
            else if (iCurrentBarHeight[i] > barHeight)
            {
                // erase part of the bar
                painter.fillRect(drawOffsetX, frameHeight-iCurrentBarHeight[i],
                                 barWidth, iCurrentBarHeight[i]-barHeight, Qt::black);
            }
            else if (iCurrentBarHeight[i] < barHeight)
            {
                // add a part to the bar
                painter.fillRect(drawOffsetX, frameHeight-barHeight,
                                 barWidth, barHeight-iCurrentBarHeight[i], Qt::white);
            }

            drawOffsetX += barWidth + 1;
            iCurrentBarHeight[i]  = barHeight;
        }
    }
}

void SpectrumWidget::updateSpectrum(float* spectrumValues)
{
    iNewSpectrum = spectrumValues;
    iMaxValue = std::max<float>(iMaxValue, getMaxValue(spectrumValues));

    //update();
}

void SpectrumWidget::audioData(short* buf, unsigned int)
{
    float* spectrum = iFourierFilter.getSpectrum(buf);

    // 'buf' has been used to generate the spectrum, continue capturing
    iMicrophoneCapture.continueCapturing();

    updateSpectrum(spectrum);
}

float SpectrumWidget::getMaxValue(float* spectrumValues)
{
    float maxValue = 0.0f;
    for (int i = 0; i < iSpectrumSize; i++)
    {
        maxValue = std::max<float>(spectrumValues[i], maxValue);
    }

    return maxValue;
}
