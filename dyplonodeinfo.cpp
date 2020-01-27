#include "dyplonodeinfo.h"
#include <QSettings>

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

void parseDyploConfig(QVector<DyploNodeInfo> *result)
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
