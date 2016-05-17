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
#include <iostream>
#include <stdio.h>

CpuInfo::CpuInfo()
{
    FILE* file = fopen("/proc/stat", "r");
    if (file)
    {
        unsigned long long totalUser, totalUserLow, totalSys;
        fscanf(file, "cpu %Ld %Ld %Ld %Ld", &totalUser, &totalUserLow,
           &totalSys, &lastTotalIdle);
        fclose(file);
        lastTotalActive = totalUser + totalUserLow + totalSys;
    }
}

int CpuInfo::getCurrentValue()
{
    int percent;
    FILE* file;
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, totalActive;

    file = fopen("/proc/stat", "r");
    if (!file)
        return -1;
    fscanf(file, "cpu %Ld %Ld %Ld %Ld", &totalUser, &totalUserLow,
           &totalSys, &totalIdle);
    fclose(file);

    totalActive = totalUser + totalUserLow + totalSys;
    if (totalActive < lastTotalActive)
    {
        //Overflow detection. Just skip this value.
        percent = -1;
    }
    else
    {
        unsigned long long active = totalActive - lastTotalActive;
        percent = (int)((active * 100) / (active + (totalIdle - lastTotalIdle)));
    }

    lastTotalActive = totalActive;
    lastTotalIdle = totalIdle;

    return percent;
}
