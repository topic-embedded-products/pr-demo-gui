#include "dyplonodeinfo.h"
#include <QDebug>
#include <iostream>
#include <fstream>

int parseDyploConfig(QVector<DyploNodeInfo> *result)
{
    std::ifstream f;
    f.open("/usr/share/floorplan-config");
    if (!f.is_open())
        f.open("floorplan-config");
    if (!f.is_open())
    {
        return -1;
    }
    while (!f.eof())
    {
       DyploNodeInfo info;
       f >> info;
       if (f.good())
           result->push_back(info);
    }
    return 0;
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
