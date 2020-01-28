#ifndef DYPLONODEINFO_H
#define DYPLONODEINFO_H

#include <QRect>
#include <QVector>

#include <ostream>
#include <istream>
#include <string>

class QLabel;

struct DyploNodeInfo
{
public:
    enum DyploNodeType {CPU, FIXED, PR, DMA, ICAP};

    int id;
    QRect geometry;
    DyploNodeType type;
    std::string function;
    QLabel *widget;

    DyploNodeInfo(int i, const QRect &g, DyploNodeType t, const std::string& f = ""):
        id(i), geometry(g), type(t), function(f), widget(nullptr)
    {}
    DyploNodeInfo():
        id(-1), type(PR), widget(nullptr)
    {}
};

void parseDyploConfig(QVector<DyploNodeInfo> *result);

std::ostream& operator<<(std::ostream& os, const DyploNodeInfo& n);
std::istream& operator>>(std::istream& is, DyploNodeInfo& n);

#endif // DYPLONODEINFO_H
