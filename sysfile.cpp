/*
 * sysfile.cpp
 *
 * Library for medical signal processing using Dyplo.
 *
 * (C) Copyright 2014,2015 Topic Embedded Products B.V. (http://www.topicembeddedproducts.nl).
 * All rights reserved.
 *
 * This file is part of medical-demo-lib.
 * medical-demo-lib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * medical-demo-lib is distributed in the hope that it will be useful,
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
#include <stdio.h>
#include <unistd.h>
#include <dyplo/exceptions.hpp>
#include <dyplo/directoryio.hpp>
#include "sysfile.hpp"

static const char IIO_DIR[] = "/sys/bus/iio/devices";
static const char HWMON_DIR[] = "/sys/class/hwmon";

int read_sys_file(const char* filename)
{
    int result;
    FILE* f = fopen(filename, "r");
    if (f == NULL)
        throw dyplo::IOException(filename);
    int err = fscanf(f, "%d", &result);
    fclose(f);
    if (err != 1)
    {
        if (err < 0)
            throw dyplo::IOException(filename);
        throw std::runtime_error("Failed to parse file");
    }
    return result;
}

float read_sys_file_float(const char* filename)
{
    float result;
    FILE* f = fopen(filename, "r");
    if (f == NULL)
        throw dyplo::IOException(filename);
    int err = fscanf(f, "%f", &result);
    fclose(f);
    if (err != 1)
    {
        if (err < 0)
            throw dyplo::IOException(filename);
        throw std::runtime_error("Failed to parse file");
    }
    return result;
}

void write_sys_file(const char* filename, int value)
{
    FILE* f = fopen(filename, "w");
    if (f == NULL)
        throw dyplo::IOException(filename);
    int result = fprintf(f, "%d", value);
    fclose(f);
    if (result < 0)
        throw dyplo::IOException(filename);
}

IIOTempSensor::IIOTempSensor():
    offset(0),
    scale(1.0f)
{
    dyplo::DirectoryListing dir(IIO_DIR);
    struct dirent *entry;
    while (((entry = dir.next()) != NULL) && filename_raw.empty())
    {
        if (entry->d_name[0] == '.')
            continue; /* Skip hidden and parent directory */
        switch (entry->d_type)
        {
            case DT_DIR:
            case DT_LNK:
            case DT_UNKNOWN:
                /* subdir */
                std::string path(IIO_DIR);
                path += "/";
                path += entry->d_name;
                path += "/";
                try
                {
                    filename_raw = path + "in_temp0_raw";
                    if (access(filename_raw.c_str(), R_OK) != 0)
                    {
                        filename_raw.clear();
                        break;
                    }
                    std::string param(path);
                    param += "in_temp0_offset";
                    offset = read_sys_file(param.c_str());
                    param = path;
                    param += "in_temp0_scale";
                    scale = read_sys_file_float(param.c_str());

                }
                catch (...)
                {}
                break;
        }
    }
}

int IIOTempSensor::getTempMilliDegrees()
{
    int raw = read_sys_file(filename_raw.c_str());
    return (int)((raw + offset) * scale);
}

SupplyCurrentSensor::SupplyCurrentSensor()
{
    dyplo::DirectoryListing dir(HWMON_DIR);
    struct dirent *entry;
    while ((entry = dir.next()) != NULL)
    {
        switch (entry->d_type)
        {
            case DT_REG:
            case DT_LNK:
            case DT_UNKNOWN:
                /* subdir */
                std::string path(HWMON_DIR);
                path += "/";
                path += entry->d_name;
                path += "/";
                filename_cpu = path + "curr1_input";
                if (access(filename_cpu.c_str(), R_OK) != 0)
                {
                    filename_cpu.clear();
                    break;
                }
                filename_fpga = path + "curr2_input";
                break;
        }
    }
}

int SupplyCurrentSensor::read_cpu_supply_current_mA()
{
    /* returns difference in uV over 5 mOhm, convert to mA */
    return read_sys_file(filename_cpu.c_str()) / 5;
}

int SupplyCurrentSensor::read_fpga_supply_current_mA()
{
    /* returns difference in uV over 5 mOhm, convert to mA */
    return read_sys_file(filename_fpga.c_str()) / 5;
}
