#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>

class SpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget *parent = 0);
    virtual ~SpectrumWidget();

    virtual void paintEvent(QPaintEvent *event);

    void setSpectrumSize(int size);
    void updateSpectrum(float* spectrumValues);
    float getMaxValue(float* spectrumValues);

signals:

public slots:

private:
    int     iSpectrumSize;
    float   iMaxValue;
    bool    iKeepPainting;

    float*  iNewSpectrum;
    bool    iDrawnCurrentBars;
    int*    iCurrentBarHeight;
};

#endif // SPECTRUMWIDGET_H
