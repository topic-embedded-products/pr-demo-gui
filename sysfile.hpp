/*
 * sysfile.hpp
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
#include <string>

int read_sys_file(const char* filename);
void write_sys_file(const char* filename, int value);

class IIOTempSensor
{
protected:
    int offset;
    float scale;
    std::string filename_raw;
public:
    IIOTempSensor(const char *inputname);
    int getTempMilliDegrees();
};

class SupplyCurrentSensor
{
protected:
    std::string filename_cpu;
    std::string filename_fpga;
public:
    SupplyCurrentSensor();
    int read_cpu_supply_current_mA();
    int read_fpga_supply_current_mA();
};
