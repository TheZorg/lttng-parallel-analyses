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

#ifndef IOCONTEXT_H
#define IOCONTEXT_H

#include "cpu/cpucontext.h"

#include <trace/BasicTypes.hpp>

#include <boost/optional.hpp>

enum class IOType { UNKNOWN, READ, WRITE, READWRITE };
static const std::string IOTypeStrings[] = { "UNKNOWN", "READ", "WRITE", "READWRITE" };

struct Syscall {
    IOType type = IOType::UNKNOWN;
    std::string name {};
    uint64_t start {};
    uint64_t end {};
    int fd = -1;
    int ret = -1;
};

struct PseudoChunk {
    boost::optional<Syscall> currentSyscall {};
    boost::optional<Syscall> unknownSyscall {};
    uint64_t end {};
};

struct IoProcess : public Process
{
    std::vector<PseudoChunk> pseudoChunks {};

    uint64_t totalReadLatency {};
    uint64_t readCount {};

    uint64_t totalWriteLatency {};
    uint64_t writeCount {};

    uint64_t readBytes {};
    uint64_t writeBytes {};
    IoProcess() { pseudoChunks.emplace_back(); }
};

class IoContext
{
public:
    IoContext();

    void handleSysRead(const tibee::trace::EventValue &event);
    void handleSysWrite(const tibee::trace::EventValue &event);
    void handleSysReadWrite(const tibee::trace::EventValue &event);
    void handleExitSyscall(const tibee::trace::EventValue &event);
    void handleSchedMigrate(const tibee::trace::EventValue &event);
    void handleEnd();

    void merge(const IoContext &other);

    const std::list<IoProcess> &getTidsByWrite();
    const std::list<IoProcess> &getTidsByRead();

    void mergeSyscalls(IoProcess &thisProcess, PseudoChunk &thisProcessChunk, const PseudoChunk &otherProcessChunk);
private:
    typedef QHash<int, IoProcess> IoProcessMap;
    IoProcessMap tids;
    std::list<IoProcess> sortedTids;
    void handleReadWrite(const tibee::trace::EventValue &event, IOType type);
};

#endif // IOCONTEXT_H
