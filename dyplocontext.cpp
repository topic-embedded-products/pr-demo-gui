#include "dyplocontext.h"
#include <errno.h>
#include <QElapsedTimer>
#include <QDebug>


static unsigned int count_numbered_files(const char* pattern)
{
    unsigned int result = 0;
    char filename[64];
    for (;;)
    {
        sprintf(filename, pattern, result);
        if (::access(filename, F_OK) != 0)
            return result;
        ++result;
    }
}

static unsigned int get_dyplo_dma_node_count()
{
    return count_numbered_files("/dev/dyplod%d");
}


/* Nodes in logic 7030:
 * 0 CPU
 * 1 CPU
 * 2 MUX
 * 3 MUX
 * 4..11 PR
 * 12..14 DMA
 * 15 ICAP
 */

static void use7030config(QVector<DyploNodeInfo> *result)
{
    result->push_back(DyploNodeInfo(0, QRect(110,  90, 91, 41), DyploNodeInfo::CPU));
    result->push_back(DyploNodeInfo(1, QRect(110, 140, 91, 41), DyploNodeInfo::CPU));

    result->push_back(DyploNodeInfo(2, QRect(330, 430, 71, 81), DyploNodeInfo::FIXED, "stream_mux"));
    result->push_back(DyploNodeInfo(3, QRect(330, 520, 71, 81), DyploNodeInfo::FIXED, "stream_mux"));

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

/* Nodes in logic 7015:
 * 0 CPU
 * 1..4 PR
 * 5..6 DMA
 * 7 ICAP
 */
static void use7015config(QVector<DyploNodeInfo> *result)
{
    result->push_back(DyploNodeInfo(0, QRect(110,  90, 91, 41), DyploNodeInfo::CPU));

    result->push_back(DyploNodeInfo(1, QRect(210, 35, 110, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(2, QRect(210, 227, 110, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(3, QRect(450, 35, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(4, QRect(450, 227, 120, 192), DyploNodeInfo::PR));

    result->push_back(DyploNodeInfo(5, QRect(110, 240, 91, 41), DyploNodeInfo::DMA));
    result->push_back(DyploNodeInfo(6, QRect(110, 290, 91, 41), DyploNodeInfo::DMA));
}

DyploContext::DyploContext():
    num_dma_nodes(get_dyplo_dma_node_count())
{
    try
    {
        hwControl = new dyplo::HardwareControl(hardwareCtx);
    }
    catch (const std::exception& ex)
    {
        qCritical() << "ERROR: Cannot initialize Dyplo:\n" << QString(ex.what());
    }
    if (parseDyploConfig(&nodeInfo))
    {
        qWarning() << "/usr/share/floorplan-config not found, using defaults for "
                   << ((num_dma_nodes > 2) ? "7030" : "7015");
        if (num_dma_nodes > 2)
            use7030config(&nodeInfo);
        else
            use7015config(&nodeInfo);
    }
}

DyploContext::~DyploContext()
{
    delete hwControl;
}

dyplo::HardwareConfig *DyploContext::createConfig(const char *name)
{
    if (!hwControl)
        throw dyplo::IOException(name, ENODEV);

    /* Check the configuration to see if there's a fixed node that can supply it */
    for (const auto& node: nodeInfo)
    {
        if (node.type == DyploNodeInfo::FIXED && node.function == name)
        {
            int handle = hardwareCtx.openConfig(node.id, O_RDWR);
            if (handle != -1)
                return new dyplo::HardwareConfig(handle);
        }
    }

    /* Find a matching PR node */
    unsigned int candidates = hardwareCtx.getAvailablePartitions(name);
    if (candidates == 0)
        throw dyplo::IOException(name, ENODEV);
    int id = 0;
    while (candidates)
    {
        if ((candidates & 0x01) != 0)
        {
            int handle = hardwareCtx.openConfig(id, O_RDWR);
            if (handle == -1)
            {
                if (errno != EBUSY) /* Non existent? Bail out, last node */
                    throw dyplo::IOException(name);
            }
            else
            {
                hwControl->disableNode(id);
                std::string filename = hardwareCtx.findPartition(name, id);
                unsigned int size;
                QElapsedTimer myTimer; /* TODO: More accurate */
                myTimer.start();
                {
                    dyplo::HardwareProgrammer programmer(hardwareCtx, *hwControl);
                    size = programmer.fromFile(filename.c_str());
                }
                unsigned int elapsed = myTimer.nsecsElapsed() / 1000;
                emit programmedPartial(id, name, size, elapsed);
                return new dyplo::HardwareConfig(handle);
            }
        }
        ++id;
        candidates >>= 1;
    }
    throw dyplo::IOException(name, ENODEV);
}

dyplo::HardwareDMAFifo *DyploContext::createDMAFifo(int access)
{
    return new dyplo::HardwareDMAFifo(hardwareCtx.openAvailableDMA(access));
}
