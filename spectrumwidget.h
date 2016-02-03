#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>
#include "microphonecapturethread.h"
#include "fourierfilter.h"

class SpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget *parent = 0);
    virtual ~SpectrumWidget();

    virtual void paintEvent(QPaintEvent *event);
    virtual bool eventFilter(QObject *object, QEvent *event);

public slots:
    void updateSpectrum(float* spectrumValues);

    void audioData(short* buf, unsigned int len);

private:
    float getMaxValue(float* spectrumValues);

    static const unsigned int SPECTRUM_SIZE;

    int     iSpectrumSize;
    float   iMaxValue;
    bool    iKeepPainting;

    float*  iNewSpectrum;
    bool    iDrawnCurrentBars;
    int*    iCurrentBarHeight;

    MicrophoneCaptureThread iMicrophoneCapture;
    FourierFilter iFourierFilter;
};

#endif // SPECTRUMWIDGET_H
