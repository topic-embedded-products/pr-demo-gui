#ifndef DYPLORESOURCES_H
#define DYPLORESOURCES_H

#include <QVector>

struct DyploNodeResource
{
    int id; /* Node ID */
    const char* name; /* Descriptive name */

    DyploNodeResource(int _id, const char* _name = NULL):
        id(_id), name(_name)
    {}

    DyploNodeResource() {} /* For use in vector et al */
};

typedef QVector<DyploNodeResource> DyploNodeResourceList;

#endif // DYPLORESOURCES_H
