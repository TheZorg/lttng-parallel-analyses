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

#ifndef CPUCONTEXT_H
#define CPUCONTEXT_H

#include <trace/value/EventValue.hpp>
#include <boost/optional.hpp>
#include <QHash>

static const int UNKNOWN_TID = -1;

struct Task {
    uint64_t start = 0;
    uint64_t end = 0;
    int tid = UNKNOWN_TID;
};

struct Process
{
    int pid = UNKNOWN_TID;
    int tid = UNKNOWN_TID;
    uint64_t cpu_ns = 0;
    std::string comm = "";
};

struct Cpu
{
    unsigned int id;
    boost::optional<Task> currentTask;
    boost::optional<Task> unknownTask;
    uint64_t cpu_ns = 0;
    Cpu(int id) : id(id) {}
};

class CpuContext
{
public:
    CpuContext();
    void handleSchedSwitch(const tibee::trace::EventValue &event);
    void handleEnd();

    void merge(const CpuContext &other);

    uint64_t getStart() const;
    void setStart(const uint64_t &value);

    uint64_t getEnd() const;
    void setEnd(const uint64_t &value);

    const std::vector<Cpu> &getCpus() const;

    const std::list<Process> &getTids() const;

private:
    Cpu& getCpu(unsigned int cpu);

private:
    std::vector<Cpu> cpus;
    QHash<int, Process> tids;
    std::list<Process> sortedTids;
    uint64_t start = 0;
    uint64_t end = 0;
};

#endif // CPUCONTEXT_H
