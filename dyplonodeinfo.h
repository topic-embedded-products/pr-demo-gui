#ifndef DYPLONODEINFO_H
#define DYPLONODEINFO_H

#include <QRect>
#include <QVector>
#include <QString>

class QLabel;

struct DyploNodeInfo
{
public:
    enum DyploNodeType {CPU, FIXED, PR, DMA, ICAP};

    int id;
    QRect geometry;
    DyploNodeType type;
    QString function;
    QLabel *widget;

    DyploNodeInfo(int i, const QRect &g, DyploNodeType t, const QString& f = ""):
        id(i), geometry(g), type(t), function(f), widget(NULL)
    {}
    DyploNodeInfo():
        id(-1), type(PR), widget(NULL)
    {}
};

void parseDyploConfig(QVector<DyploNodeInfo> *result);

#endif // DYPLONODEINFO_H
