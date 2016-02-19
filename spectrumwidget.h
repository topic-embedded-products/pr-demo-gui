#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>
#include "fourierfilter.h"

class SpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget *parent = 0);
    virtual ~SpectrumWidget();

    virtual void paintEvent(QPaintEvent *event);

public slots:
    void updateSpectrum(float* spectrumValues);

    void audioData(short* buf, unsigned int len);

private:
    float   iMaxValue;
    bool    iKeepPainting;

    float*  iNewSpectrum;
    bool    iDrawnCurrentBars;
    int*    iCurrentBarHeight;

    FourierFilter iFourierFilter;
};

#endif // SPECTRUMWIDGET_H
