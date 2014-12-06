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

#include "iocontext.h"

IoContext::IoContext()
{
}

void IoContext::handleSysRead(const tibee::trace::EventValue &event)
{
    handleReadWrite(event, IOType::READ);
}

void IoContext::handleSysWrite(const tibee::trace::EventValue &event)
{
    handleReadWrite(event, IOType::WRITE);
}

void IoContext::handleExitSyscall(const tibee::trace::EventValue &event)
{
    uint64_t timestamp = event.getTimestamp();
    if (!event.getStreamEventContext()->HasField("tid")) {
        std::cerr << "Missing tid context info" << std::endl;
        return;
    }
    int tid = event.getStreamEventContext()->GetField("tid")->AsInteger();
    int64_t ret = event.getFields()->GetField("ret")->AsLong();

    if (!tids.contains(tid)) {
        IoProcess p;
        p.tid = tid;
        tids[tid] = p;
    }
    IoProcess &p = tids[tid];
    if (!p.currentSyscall) {
        if (!p.unknownSyscall) {
            // We have an unkown syscall, save it
            p.unknownSyscall = Syscall();
            p.unknownSyscall->end = timestamp;
            p.unknownSyscall->ret = ret;
        }
    } else {
        if (ret >= 0) {
            uint64_t latency = timestamp - p.currentSyscall->start;
            switch (p.currentSyscall->type) {
            case IOType::READ:
                if (tid == 13360) {
                    std::cout << "ding" << std::endl;
                }
                p.totalReadLatency += latency;
                p.readBytes += ret;
                p.readCount++;
                break;
            case IOType::WRITE:
                p.totalWriteLatency += latency;
                p.writeBytes += ret;
                p.writeCount++;
                break;
            default:
                break;
            }
        }
        p.currentSyscall = boost::none;
    }
}
const std::list<IoProcess> &IoContext::getTidsByWrite()
{
    sortedTids.sort([](const IoProcess &a, const IoProcess &b) -> bool {
        return a.writeBytes > b.writeBytes;
    });
    return sortedTids;
}

const std::list<IoProcess> &IoContext::getTidsByRead()
{
    sortedTids.sort([](const IoProcess &a, const IoProcess &b) -> bool {
        return a.readBytes > b.readBytes;
    });
    return sortedTids;
}

void IoContext::handleEnd()
{
    QList<IoProcess> l = tids.values();
    sortedTids = l.toStdList();
}

void IoContext::handleReadWrite(const tibee::trace::EventValue &event, IOType type)
{
    uint64_t timestamp = event.getTimestamp();
    std::string name = event.getName();
    if (!event.getStreamEventContext()->HasField("tid")) {
        std::cerr << "Missing tid context info" << std::endl;
        return;
    }
    int tid = event.getStreamEventContext()->GetField("tid")->AsInteger();
    int fd = event.getFields()->GetField("fd")->AsInteger();

    if (!tids.contains(tid)) {
        IoProcess p;
        p.tid = tid;
        tids[tid] = p;
    }
    IoProcess &p = tids[tid];
    p.currentSyscall = Syscall();
    p.currentSyscall->type = type;
    p.currentSyscall->start = timestamp;
    p.currentSyscall->fd = fd;
    p.currentSyscall->name = name;
}
