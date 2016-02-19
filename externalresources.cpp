#include "externalresources.h"
#include "dyplocontext.h"
#include <dyplo/hardware.hpp>

ExternalResources::ExternalResources(QObject *parent) : QObject(parent)
{
}

ExternalResources::~ExternalResources()
{
    for (int i = 0; i < resources.size(); ++i)
        if (resources[i])
            delete resources[i];
}

bool ExternalResources::aquire(DyploContext *dyplo, int number)
{
    if (number >= resources.size())
        resources.resize(number+1);
    dyplo::HardwareConfig *cur = resources[number];
    if (cur)
        return true; /* Already taken? */
    int fd = dyplo->GetHardwareContext().openConfig(number, O_RDWR);
    if (fd == -1)
        return false;
    resources[number] = new dyplo::HardwareConfig(fd);
    return resources[number] != NULL;
}

void ExternalResources::release(int number)
{
    if (number < resources.size()) {
        delete resources[number];
        resources[number] = NULL;
    }
}

bool ExternalResources::isAquired(int number)
{
    if (number >= resources.size())
        return false;
    return resources[number] != NULL;
}

void ExternalResources::enumDyploResources(DyploNodeInfoList &list)
{
    for (int i = 0; i < resources.size(); ++i)
        if (resources[i])
            list.push_back(DyploNodeInfo(resources[i]->getNodeIndex(), "X"));
}
