#ifndef QPRREGIONLABEL_H
#define QPRREGIONLABEL_H

#include <QLabel>

class QPRRegionLabel : public QLabel
{
    Q_OBJECT

public:
    QPRRegionLabel(QWidget *parent = 0);

protected slots:
    virtual void mousePressEvent(QMouseEvent * event);
};

#endif
