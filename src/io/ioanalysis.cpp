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
#include "common/packetindex.h"

#include <QtConcurrent>
#include <QTemporaryDir>
#include <QUuid>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>

std::vector<std::string> readSyscalls = {"sys_read", "syscall_entry_read",
                              "sys_recvmsg", "syscall_entry_recvmsg",
                              "sys_recvfrom", "syscall_entry_recvfrom",
                              "sys_readv", "syscall_entry_readv"};

std::vector<std::string> writeSyscalls = {"sys_write", "syscall_entry_write",
                               "sys_sendmsg", "syscall_entry_sendmsg",
                               "sys_sendto", "syscall_entry_sendto",
                               "sys_writev", "syscall_entry_writev"};

std::vector<std::string> readWriteSyscalls = {"sys_splice", "syscall_entry_splice",
                                              "sys_sendfile64", "syscall_entry_sendfile64"};

std::vector<std::string> exitSyscalls = {"syscall_exit_read",
                                         "syscall_exit_recvmsg",
                                         "syscall_exit_recvfrom",
                                         "syscall_exit_readv",
                                         "syscall_exit_write",
                                         "syscall_exit_sendmsg",
                                         "syscall_exit_sendto",
                                         "syscall_exit_writev",
                                         "syscall_exit_splice",
                                         "syscall_exit_sendfile64",
                                         "exit_syscall"};

IoWorker::IoWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose) :
    TraceWorker(id, set, begin, end, verbose)
{
}

IoContext IoWorker::doMap() const
{
    const TraceSet &set = getTraceSet();
    TraceSet::Iterator iter = set.between(getBeginPos(), getEndPos());
    TraceSet::Iterator endIter = set.end();

    IoContext data;

    // Get event ids
    std::set<event_id_t> readEventIds {};
    for (const std::string &eventName : readSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readEventIds.insert(id);
    }
    std::set<event_id_t> writeEventIds {};
    for (const std::string &eventName : writeSyscalls) {
        event_id_t id = getEventId(set, eventName);
        writeEventIds.insert(id);
    }
    std::set<event_id_t> readWriteEventIds {};
    for (const std::string &eventName : readWriteSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readWriteEventIds.insert(id);
    }
    std::set<event_id_t> exitEventIds {};
    for (const std::string &eventName : exitSyscalls) {
        event_id_t id = getEventId(set, eventName);
        exitEventIds.insert(id);
    }

    event_id_t migrate_id = getEventId(set, "sched_migrate_task");

    // Iterate through events
    uint64_t count = 0;
    for ((void)iter; iter != endIter; ++iter) {
        count++;
        const auto &event = *iter;
        event_id_t id = event.getId();
        if (readEventIds.find(id) != readEventIds.end()) {
            data.handleSysRead(event);
        } else if (writeEventIds.find(id) != writeEventIds.end()) {
            data.handleSysWrite(event);
        } else if (readWriteEventIds.find(id) != readWriteEventIds.end()) {
            data.handleSysReadWrite(event);
        } else if (exitEventIds.find(id) != exitEventIds.end()) {
            data.handleExitSyscall(event);
        } else if (id == migrate_id) {
            data.handleSchedMigrate(event);
        }
    }

    if (getVerbose()) {
        const timestamp_t *begin = getBeginPos();
        const timestamp_t *end = getEndPos();
        std::string beginString = begin ? std::to_string(*begin) : "START";
        std::string endString = end ? std::to_string(*end) : "END";
        std::cout << "Worker " << getId() << " processed " << count << " events between timestamps "
                  << beginString << " and " << endString << std::endl;
    }

    data.handleEnd();

    return data;
}

void IoWorker::doReduce(IoContext &final, const IoContext &intermediate)
{
    final.merge(intermediate);
}

bool IoAnalysis::isOrderedReduce()
{
    return true;
}

void IoAnalysis::doExecuteSerial()
{
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    IoContext data;

    // Get event ids
    std::set<event_id_t> readEventIds {};
    for (const std::string &eventName : readSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readEventIds.insert(id);
    }
    std::set<event_id_t> writeEventIds {};
    for (const std::string &eventName : writeSyscalls) {
        event_id_t id = getEventId(set, eventName);
        writeEventIds.insert(id);
    }
    std::set<event_id_t> readWriteEventIds {};
    for (const std::string &eventName : readWriteSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readWriteEventIds.insert(id);
    }

    std::set<event_id_t> exitEventIds {};
    for (const std::string &eventName : exitSyscalls) {
        event_id_t id = getEventId(set, eventName);
        exitEventIds.insert(id);
    }
    // Iterate through events
    for (const auto &event : set) {
        event_id_t id = event.getId();
        if (readEventIds.find(id) != readEventIds.end()) {
            data.handleSysRead(event);
        } else if (writeEventIds.find(id) != writeEventIds.end()) {
            data.handleSysWrite(event);
        } else if (readWriteEventIds.find(id) != readWriteEventIds.end()) {
            data.handleSysReadWrite(event);
        } else if (exitEventIds.find(id) != exitEventIds.end()) {
            data.handleExitSyscall(event);
        }
    }

    data.handleEnd();

    printResults(data);
}

void IoAnalysis::printResults(IoContext &data)
{
    std::string line(80, '-');
    int max = 10;
    int count = 0;
    int colWidth = 30;

    std::cout << line << std::endl;
    std::cout << "Result of I/O analysis" << std::endl << std::endl;
    std::cout << "Syscall I/O Read" << std::endl << std::endl;
    std::cout << std::setw(colWidth) << std::left << "Process" <<
                 std::setw(colWidth) << std::left << "Size" << std::endl;

    for (const IoProcess &process : data.getTidsByRead()) {
        if (++count > max) {
            break;
        }
        std::stringstream ss;
        ss << process.comm << " (" << process.tid << ")";
        std::cout << std::setw(colWidth) << std::left << ss.str() <<
                     std::setw(colWidth) << std::left << convertSize(process.readBytes) << std::endl;
    }
    std::cout << line << std::endl;
    std::cout << "Syscall I/O Write" << std::endl << std::endl;
    std::cout << std::setw(colWidth) << std::left << "Process" <<
                 std::setw(colWidth) << std::left << "Size" << std::endl;
    count = 0;
    for (const IoProcess &process : data.getTidsByWrite()) {
        if (++count > max) {
            break;
        }
        std::stringstream ss;
        ss << process.comm << " (" << process.tid << ")";
        std::cout << std::setw(colWidth) << std::left << ss.str() <<
                     std::setw(colWidth) << std::left << convertSize(process.writeBytes) << std::endl;
    }
}

void IoAnalysis::doEnd(IoContext &data)
{
    data.handleEnd();
}
