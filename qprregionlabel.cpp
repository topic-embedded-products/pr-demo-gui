#include "qprregionlabel.h"

QPRRegionLabel::QPRRegionLabel(QWidget *parent):
    QLabel(parent)
{
}

void QPRRegionLabel::mousePressEvent(QMouseEvent *)
{
    emit linkActivated(objectName());
}
