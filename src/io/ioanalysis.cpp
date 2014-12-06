/* Copyright (c) 2014 Fabien Reumont-Locke <fabien.reumont-locke@polymtl.ca>
 *
 * This file is part of lttng-parallel-analyses.
 *
 * lttng-parallel-analyses is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * lttng-parallel-analyses is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lttng-parallel-analyses.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ioanalysis.h"
#include "iocontext.h"
#include "common/utils.h"

#include <iostream>
#include <iomanip>
#include <sstream>


void IoAnalysis::doExecuteParallel()
{

}

void IoAnalysis::doExecuteSerial()
{
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    IoContext data;

    // Get event ids
    event_id_t sysReadId = getEventId(set, "sys_read");
    event_id_t sysReadVId = getEventId(set, "sys_readv");
    event_id_t sysWriteId = getEventId(set, "sys_write");
    event_id_t sysWriteVId = getEventId(set, "sys_writev");
    event_id_t exitSyscallId = getEventId(set, "exit_syscall");

    if (!sysReadId | !sysReadVId | !sysWriteId | !sysWriteVId | !exitSyscallId) {
        std::cerr << "The trace is missing events." << std::endl;
        return;
    }

    // Iterate through events
    for (const auto &event : set) {
        event_id_t id = event.getId();
        if (id == sysReadId || id == sysReadVId) {
            data.handleSysRead(event);
        } else if (id == sysWriteId || id == sysWriteVId) {
            data.handleSysWrite(event);
        } else if (id == exitSyscallId) {
            data.handleExitSyscall(event);
        }
    }

    data.handleEnd();

    printResults(data);
}

void IoAnalysis::printResults(IoContext &data)
{
    std::string line(80, '-');

    std::cout << line << std::endl;
    std::cout << "Result of I/O analysis" << std::endl << std::endl;
    std::cout << "Syscall I/O Read" << std::endl << std::endl;
    std::cout << std::setw(20) << std::left << "Process" <<
                 std::setw(20) << std::left << "Size" << std::endl;

    int max = 10;
    int count = 0;
    for (const IoProcess &process : data.getTidsByRead()) {
        if (++count > max) {
            break;
        }
        std::stringstream ss;
        ss << process.comm << " (" << process.tid << ")";
        std::cout << std::setw(20) << std::left << ss.str() <<
                     std::setw(20) << std::left << convertSize(process.readBytes) << std::endl;
        if (process.tid == 13360) {
            std::cout << "ding" << std::endl;
        }
    }
    std::cout << line << std::endl;
    std::cout << "Syscall I/O Write" << std::endl << std::endl;
    std::cout << std::setw(20) << std::left << "Process" <<
                 std::setw(20) << std::left << "Size" << std::endl;
    count = 0;
    for (const IoProcess &process : data.getTidsByWrite()) {
        if (++count > max) {
            break;
        }
        std::stringstream ss;
        ss << process.comm << " (" << process.tid << ")";
        std::cout << std::setw(20) << std::left << ss.str() <<
                     std::setw(20) << std::left << convertSize(process.writeBytes) << std::endl;
    }
}
