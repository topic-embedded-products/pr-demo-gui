/*
 * cpuinfo.cpp
 *
 * Dyplo Demonstration application.
 *
 * (C) Copyright 2013,2014 Topic Embedded Products B.V. <Mike Looijmans> (http://www.topic.nl).
 * All rights reserved.
 *
 * This file is part of dyplo-demo.
 * dyplo-demo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dyplo-demo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with <product name>.  If not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA or see <http://www.gnu.org/licenses/>.
 *
 * You can contact Topic by electronic mail via info@topic.nl or via
 * paper mail at the following address: Postbus 440, 5680 AK Best, The Netherlands.
 */
#include "cpuinfo.h"
#include <QDebug>

CpuInfo::CpuInfo()
{
    struct tms lastTms;
    lastTicks = times(&lastTms);
    prev_utime = lastTms.tms_utime;
    prev_stime = lastTms.tms_stime;
}

int CpuInfo::getCurrentValue()
{
    struct tms nowTms;
    time_t ticks = times(&nowTms);

    if (ticks == (time_t)-1)
        return -1; /* error? */

    time_t used_ticks = (nowTms.tms_utime - prev_utime) + (nowTms.tms_stime - prev_stime);
    time_t total_ticks = ticks - lastTicks;

    lastTicks = ticks;

    if (total_ticks <= 0)
        return -1; /* No time elapsed? */

    prev_utime = nowTms.tms_utime;
    prev_stime = nowTms.tms_stime;

    return ((100 * used_ticks) + 50) / total_ticks; /* Round up */
}
