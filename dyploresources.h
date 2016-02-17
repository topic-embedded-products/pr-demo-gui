#ifndef DYPLORESOURCES_H
#define DYPLORESOURCES_H

#include <QVector>

struct DyploNodeInfo
{
    int id; /* Node ID */
    const char* name; /* Descriptive name */

    DyploNodeInfo(int _id, const char* _name = NULL):
        id(_id), name(_name)
    {}

    DyploNodeInfo() {} /* For use in vector et al */
};

typedef QVector<DyploNodeInfo> DyploNodeInfoList;

#endif // DYPLORESOURCES_H
