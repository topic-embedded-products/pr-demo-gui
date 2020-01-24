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
    result->push_back(DyploNodeInfo(0, QRect(120, 130, 91, 41), DyploNodeInfo::CPU));
    result->push_back(DyploNodeInfo(1, QRect(120, 180, 91, 41), DyploNodeInfo::CPU));

    result->push_back(DyploNodeInfo(2, QRect(340, 470, 71, 81), DyploNodeInfo::FIXED, "mux"));
    result->push_back(DyploNodeInfo(3, QRect(340, 560, 71, 81), DyploNodeInfo::FIXED, "mux"));

    result->push_back(DyploNodeInfo(4, QRect(220, 75, 110, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(5, QRect(220, 267, 110, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(6, QRect(460, 75, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(7, QRect(460, 267, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(8, QRect(460, 459, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(9, QRect(436, 653, 142, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(10, QRect(194, 653, 120, 192), DyploNodeInfo::PR));
    result->push_back(DyploNodeInfo(11, QRect(60, 653, 114, 192), DyploNodeInfo::PR));

    result->push_back(DyploNodeInfo(12, QRect(120, 280, 91, 41), DyploNodeInfo::DMA));
    result->push_back(DyploNodeInfo(13, QRect(120, 330, 91, 41), DyploNodeInfo::DMA));
    result->push_back(DyploNodeInfo(14, QRect(120, 380, 91, 41), DyploNodeInfo::DMA));
}
