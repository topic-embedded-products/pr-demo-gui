#include "spectrumwidget.h"

#include <QPainter>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <QTime>
#include <cmath>

SpectrumWidget::SpectrumWidget(QWidget *parent) :
    QWidget(parent),
    iSpectrumSize(0),
    iMaxValue(0.0f),
    iNewSpectrum(NULL),
    iDrawnCurrentBars(false),
    iCurrentBarHeight(NULL)
{
    // force no complete redraw
    setAttribute(Qt::WA_OpaquePaintEvent);
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

void SpectrumWidget::setSpectrumSize(int size)
{
    iSpectrumSize = size;

    if (iCurrentBarHeight != NULL)
        delete[] iCurrentBarHeight;

    iCurrentBarHeight = new int[size];
    memset(iCurrentBarHeight, 0, size*sizeof(int));
}

void SpectrumWidget::updateSpectrum(float* spectrumValues)
{
    iNewSpectrum = spectrumValues;
    iMaxValue = std::max<float>(iMaxValue, getMaxValue(spectrumValues));

    update();
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
