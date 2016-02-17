#include "spectrumwidget.h"

#include <QPainter>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <QTime>
#include <cmath>
#include <QEvent>

static const unsigned int SPECTRUM_SIZE = 100;

SpectrumWidget::SpectrumWidget(QWidget *parent) :
    QWidget(parent),
    iMaxValue(0.0f),
    iNewSpectrum(NULL),
    iDrawnCurrentBars(false),
    iCurrentBarHeight(NULL),
    iMicrophoneCapture(),
    iFourierFilter(MicrophoneCaptureThread::CAPTURE_SIZE, SPECTRUM_SIZE)
{
    iCurrentBarHeight = new int[SPECTRUM_SIZE];
    memset(iCurrentBarHeight, 0, SPECTRUM_SIZE*sizeof(int));

    // force no complete redraw
    setAttribute(Qt::WA_OpaquePaintEvent);

    connect(&iMicrophoneCapture, SIGNAL(capturedAudio(short*,uint)), this, SLOT(audioData(short*,uint)));
    QString audioDeviceName("default");
    iMicrophoneCapture.startCapturing(audioDeviceName);
}

SpectrumWidget::~SpectrumWidget()
{
    delete[] iCurrentBarHeight;
}

void SpectrumWidget::paintEvent(QPaintEvent* )
{
    QPainter painter(this);

    if (iNewSpectrum != NULL)
    {
        float widthPerBar = (frameSize().width()/(float)SPECTRUM_SIZE) - 1.0;
        Q_ASSERT(widthPerBar > 0.0f);

        double intPart;
        double fractionalPart = modf(widthPerBar, &intPart);

        widthPerBar = floor(widthPerBar);
        int maxHeightPerBar = frameSize().height() * 0.95;

        int frameHeight = frameSize().height();

        int drawOffsetX = 1.0;
        double fractionalPartCounter = 0.0f;

        for (unsigned int i = 0; i < SPECTRUM_SIZE; ++i)
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

static float getMaxValue(float* spectrumValues)
{
    float maxValue = 0.0f;
    for (unsigned int i = 0; i < SPECTRUM_SIZE; i++)
    {
        maxValue = std::max<float>(spectrumValues[i], maxValue);
    }

    return maxValue;
}

void SpectrumWidget::updateSpectrum(float* spectrumValues)
{
    iNewSpectrum = spectrumValues;

    iMaxValue = std::max<float>(iMaxValue, getMaxValue(spectrumValues));

    // deduct a small percentage of the maximum (correction for loud noises).
    iMaxValue =  iMaxValue * 0.9995;

    update();
}

void SpectrumWidget::audioData(short* buf, unsigned int)
{
    float* spectrum = iFourierFilter.getSpectrum(buf);

    // 'buf' has been used to generate the spectrum, continue capturing
    iMicrophoneCapture.continueCapturing();

    updateSpectrum(spectrum);
}

