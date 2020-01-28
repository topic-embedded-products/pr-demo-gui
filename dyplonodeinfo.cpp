#include "dyplonodeinfo.h"
#include <QDebug>
#include <iostream>
#include <fstream>

/* Nodes in logic 7030:
 * 0 CPU
 * 1 CPU
 * 2 MUX
 * 3 MUX
 * 4..11 PR
 * 12..14 DMA
 * 15 ICAP
 */

/* Nodes in logic 7015:
 * 0 CPU
 * 1..4 PR
 * 5..6 DMA
 * 7 ICAP
 */

static void use7030config(QVector<DyploNodeInfo> *result)
{
    result->push_back(DyploNodeInfo(0, QRect(110,  90, 91, 41), DyploNodeInfo::CPU));
    result->push_back(DyploNodeInfo(1, QRect(110, 140, 91, 41), DyploNodeInfo::CPU));

    result->push_back(DyploNodeInfo(2, QRect(330, 430, 71, 81), DyploNodeInfo::FIXED, "mux"));
    result->push_back(DyploNodeInfo(3, QRect(330, 520, 71, 81), DyploNodeInfo::FIXED, "mux"));

    result->push_back(DyploNodeInfo(4, QRect(210, 35, 110, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(5, QRect(210, 227, 110, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(6, QRect(450, 35, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(7, QRect(450, 227, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(8, QRect(450, 419, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(9, QRect(426, 613, 142, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(10, QRect(184, 613, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(11, QRect(50, 613, 114, 192), DyploNodeInfo::PR));

    result->push_back(DyploNodeInfo(12, QRect(110, 240, 91, 41), DyploNodeInfo::DMA));
    result->push_back(DyploNodeInfo(13, QRect(110, 290, 91, 41), DyploNodeInfo::DMA));
    result->push_back(DyploNodeInfo(14, QRect(110, 340, 91, 41), DyploNodeInfo::DMA));
}

void parseDyploConfig(QVector<DyploNodeInfo> *result)
{
    std::ifstream f;
    f.open("/usr/share/floorplan-config");
    if (!f.is_open())
        f.open("floorplan-config");
    if (!f.is_open())
    {
        qWarning() << "/usr/share/floorplan-config not found, using defaults.";
        use7030config(result);
        return;
    }
    while (!f.eof())
    {
       DyploNodeInfo info;
       f >> info;
       if (f.good())
           result->push_back(info);
    }
}

std::ostream& operator<<(std::ostream& os, const DyploNodeInfo& n)
{
    os << n.id << ' ';

    switch (n.type)
    {
    case DyploNodeInfo::CPU:
        os << "CPU";
        break;
    case DyploNodeInfo::FIXED:
        os << n.function;
        break;
    case DyploNodeInfo::PR:
        os << "PR";
        break;
    case DyploNodeInfo::DMA:
        os << "DMA";
        break;
    case DyploNodeInfo::ICAP:
        os << "ICAP";
        break;
    }

    os << ' ' << n.geometry.x()
       << ' ' << n.geometry.y()
       << ' ' << n.geometry.width()
       << ' ' << n.geometry.height();

    return os;
}

std::istream& operator>>(std::istream& is, DyploNodeInfo& n)
{
    is >> n.id;
    std::string t;
    is >> t;
    if (t == std::string("CPU"))
        n.type = DyploNodeInfo::CPU;
    else if (t == std::string("PR"))
        n.type = DyploNodeInfo::PR;
    else if (t == std::string("DMA"))
        n.type = DyploNodeInfo::DMA;
    else if (t == std::string("ICAP"))
        n.type = DyploNodeInfo::ICAP;
    else
        n.type = DyploNodeInfo::FIXED;
    n.function = t;

    int x, y, w, h;
    is >> x >> y >> w >> h;
    n.geometry = QRect(x, y, w, h);

    return is;
}
