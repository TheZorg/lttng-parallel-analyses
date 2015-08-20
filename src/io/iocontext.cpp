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

void IoContext::handleSysReadWrite(const tibee::trace::EventValue &event)
{
    handleReadWrite(event, IOType::READWRITE);
}

void IoContext::handleExitSyscall(const tibee::trace::EventValue &event)
{
    uint64_t timestamp = event.getTimestamp();
    if (!event.getStreamEventContext()->HasField("tid")) {
        std::cerr << "Missing tid context info" << std::endl;
        return;
    }
    std::string comm = "";
    if (event.getStreamEventContext()->HasField("procname")) {
        comm = event.getStreamEventContext()->GetField("procname")->AsString();
    }
    int tid = event.getStreamEventContext()->GetField("tid")->AsInteger();
    int64_t ret = event.getFields()->GetField("ret")->AsLong();

    if (!tids.contains(tid)) {
        IoProcess p;
        p.tid = tid;
        p.comm = comm;
        tids[tid] = p;
    }
    IoProcess &p = tids[tid];
    PseudoChunk &c = p.pseudoChunks.back();
    if (!c.currentSyscall) {
        if (!c.unknownSyscall) {
            // We have an unkown syscall, save it
            c.unknownSyscall = Syscall();
            c.unknownSyscall->end = timestamp;
            c.unknownSyscall->ret = ret;
        }
    } else {
        if (ret >= 0) {
            uint64_t latency = timestamp - c.currentSyscall->start;
            if (c.currentSyscall->type == IOType::READ ||
                c.currentSyscall->type == IOType::READWRITE) {
                p.totalReadLatency += latency;
                p.readBytes += ret;
                p.readCount++;
            }
            if (c.currentSyscall->type == IOType::WRITE ||
                       c.currentSyscall->type == IOType::READWRITE) {
                p.totalWriteLatency += latency;
                p.writeBytes += ret;
                p.writeCount++;
            }
        }
        c.currentSyscall = boost::none;
    }
}

void IoContext::handleSchedMigrate(const tibee::trace::EventValue &event)
{
    uint64_t timestamp = event.getTimestamp();
    if (!event.getStreamEventContext()->HasField("tid")) {
        std::cerr << "Missing tid context info" << std::endl;
        return;
    }
    int tid = event.getStreamEventContext()->GetField("tid")->AsInteger();

    if (!tids.contains(tid)) {
       // Ignore
       return;
    }

    IoProcess &p = tids[tid];
    p.pseudoChunks.back().end = timestamp;
    p.pseudoChunks.emplace_back();
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
//    for (auto &i : tids) {
//        IoProcess &p = i.value();
//        p.pseudoChunks.back().end = this->
//    }
    QList<IoProcess> l = tids.values();
    sortedTids = l.toStdList();
}

void IoContext::mergeSyscalls(IoProcess &thisProcess, PseudoChunk &thisProcessChunk, const PseudoChunk &otherProcessChunk)
{
    if (thisProcessChunk.currentSyscall) {
        // We have an unfinished syscall
        if (otherProcessChunk.unknownSyscall) {
            // We need to check, in case we are at the end of the trace
            uint64_t latency = otherProcessChunk.unknownSyscall->end - thisProcessChunk.currentSyscall->start;
            if (thisProcessChunk.currentSyscall->type == IOType::READ ||
                thisProcessChunk.currentSyscall->type == IOType::READWRITE) {
                int64_t ret = otherProcessChunk.unknownSyscall->ret;
                if (ret >= 0) {
                    thisProcess.totalReadLatency += latency;
                    thisProcess.readBytes += ret;
                    thisProcess.readCount++;
                }
            }
            if (thisProcessChunk.currentSyscall->type == IOType::WRITE ||
                thisProcessChunk.currentSyscall->type == IOType::READWRITE) {
                int64_t ret = otherProcessChunk.unknownSyscall->ret;
                if (ret >= 0) {
                    thisProcess.totalWriteLatency += latency;
                    thisProcess.writeBytes += ret;
                    thisProcess.writeCount++;
                }
            }
        }
    }
    thisProcessChunk.currentSyscall = otherProcessChunk.currentSyscall;
}

void IoContext::merge(const IoContext &other)
{
    for (const IoProcess &otherProcess : other.tids.values()) {
        if (tids.contains(otherProcess.tid)) {
            IoProcess &thisProcess = tids[otherProcess.tid];

            thisProcess.totalReadLatency += otherProcess.totalReadLatency;
            thisProcess.totalWriteLatency += otherProcess.totalWriteLatency;

            thisProcess.readBytes += otherProcess.readBytes;
            thisProcess.writeBytes += otherProcess.writeBytes;

            thisProcess.readCount += otherProcess.readCount;
            thisProcess.writeCount += otherProcess.writeCount;

            thisProcess.pseudoChunks.insert(thisProcess.pseudoChunks.end(), otherProcess.pseudoChunks.begin(), otherProcess.pseudoChunks.end());


//            PseudoChunk &thisProcessChunk = thisProcess.pseudoChunks.back();
//            const PseudoChunk &otherProcessChunk = otherProcess.pseudoChunks.back();

//            mergeSyscalls(thisProcess, thisProcessChunk, otherProcessChunk);
        } else {
            tids[otherProcess.tid] = otherProcess;
        }
    }
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

    std::string comm = "";
    if (event.getStreamEventContext()->HasField("procname")) {
        comm = event.getStreamEventContext()->GetField("procname")->AsString();
    }

    if (!tids.contains(tid)) {
        IoProcess p;
        p.tid = tid;
        p.comm = comm;
        tids[tid] = p;
    }
    IoProcess &p = tids[tid];
    PseudoChunk &c = p.pseudoChunks.back();
    c.currentSyscall = Syscall();
    c.currentSyscall->type = type;
    c.currentSyscall->start = timestamp;
    c.currentSyscall->name = name;

    if (type == IOType::READ || type == IOType::WRITE) {
        int fd = event.getFields()->GetField("fd")->AsInteger();
        c.currentSyscall->fd = fd;
    } else if (type == IOType::READWRITE) {
        // TODO: fd analysis
    }

}
