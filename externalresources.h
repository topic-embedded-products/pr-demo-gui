#ifndef EXTERNALRESOURCES_H
#define EXTERNALRESOURCES_H

#include <QObject>
#include <QVector>
#include "dyploresources.h"

namespace dyplo {
class HardwareConfig;
}
class DyploContext;

class ExternalResources : public QObject
{
    Q_OBJECT
public:
    explicit ExternalResources(QObject *parent = 0);
    ~ExternalResources();

    bool aquire(DyploContext* dyplo, int number);
    void release(int number);
    bool isAquired(int number);
    void enumDyploResources(DyploNodeResourceList& list);
signals:

public slots:
protected:
    QVector<dyplo::HardwareConfig *> resources;
};

#endif // EXTERNALRESOURCES_H
