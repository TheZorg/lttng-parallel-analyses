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

#include "cpucontext.h"

#include <algorithm>

CpuContext::CpuContext()
{
}

void CpuContext::handleSchedSwitch(const tibee::trace::EventValue &event)
{
    uint64_t timestamp = event.getTimestamp();
    int cpu = event.getStreamPacketContext()->GetField("cpu_id")->AsUInteger();
    int prev_pid = event.getFields()->GetField("prev_tid")->AsInteger();
    int next_pid = event.getFields()->GetField("next_tid")->AsInteger();
    std::string prev_comm = event.getFields()->GetField("prev_comm")->AsString();

    // Calculate CPU time
    Cpu &c = getCpu(cpu);
    if (c.currentTask) {
        // We had a currently running task
        c.cpu_ns += timestamp - c.currentTask->start;
    } else if (prev_pid != 0) {
        // We had an unknown running task
        c.unknownTask = Task();
        c.unknownTask->end = timestamp;
        c.unknownTask->tid = prev_pid;
    }

    // Calculate PID time
    // Create previous process if doesn't exist
    if(!tids.contains(prev_pid)) {
        Process p;
        p.comm = prev_comm;
        p.tid = prev_pid;
        tids[prev_pid] = p;
    } else {
        Process &p = tids[prev_pid];
        p.comm = prev_comm;
    }

    // Update previous process
    if (c.currentTask) {
        Process &p = tids[prev_pid];
        p.cpu_ns += (timestamp - c.currentTask->start);
    }

    // Update current task
    if (next_pid != 0) {
        c.currentTask = Task();
        c.currentTask->start = timestamp;
        c.currentTask->tid = next_pid;
    } else {
        c.currentTask = boost::none;
    }
}

void CpuContext::handleEnd()
{
    // Patch the end tasks
    for (Cpu &cpu : cpus) {
        if (cpu.currentTask) {
            cpu.cpu_ns += end - cpu.currentTask->start;
            Process &p = tids[cpu.currentTask->tid];
            p.cpu_ns += end - cpu.currentTask->start;
            cpu.currentTask = boost::none;
        }
    }

    // Sort per CPU time
    std::sort(cpus.begin(), cpus.end(), [](const Cpu &a, const Cpu &b) -> bool {
        return a.cpu_ns > b.cpu_ns;
    });

    // Sort TIDs
    QList<Process> l = tids.values();
    sortedTids = l.toStdList();
    sortedTids.sort([](const Process &a, const Process &b) -> bool {
        return a.cpu_ns > b.cpu_ns;
    });
}

void CpuContext::merge(const CpuContext &other)
{
    // Fix start/end times
    if (other.start < start || start == 0) {
        start = other.start;
    }
    if (other.end > end) {
        end = other.end;
    }

    // Merge CPUs
    for (const Cpu &otherCpu : other.cpus) {
        Cpu &thisCpu = getCpu(otherCpu.id);
        thisCpu.cpu_ns += otherCpu.cpu_ns;
    }

    // Merge TIDs
    QHashIterator<int, Process> otherTidsIter(other.tids);
    while (otherTidsIter.hasNext()) {
        otherTidsIter.next();
        const int &tid = otherTidsIter.key();
        const Process &otherTid = otherTidsIter.value();
        if (tids.contains(tid)) {
            Process &thisTid = tids[tid];
            thisTid.cpu_ns += otherTid.cpu_ns;
            thisTid.comm = otherTid.comm;
        } else {
            tids[tid] = otherTid;
        }
    }

    // Do fixing
    for (const Cpu &otherCpu : other.cpus) {
        Cpu &thisCpu = getCpu(otherCpu.id);
        // Check for unfinished task
        if (thisCpu.currentTask) {
            // Check if matching unknown task
            if (otherCpu.unknownTask) {
                // Merge cpu time
                uint64_t taskTime = otherCpu.unknownTask->end - thisCpu.currentTask->start;
                thisCpu.cpu_ns += taskTime;
                if (thisCpu.currentTask->tid == otherCpu.unknownTask->tid) {
                    // Merge process time and name
                    Process &thisProcess = tids[thisCpu.currentTask->tid];
                    thisProcess.cpu_ns += taskTime;
                } else {
                    std::cerr << "Mismatch: merging current tid=" << thisCpu.currentTask->tid
                              << " with unknown tid=" << otherCpu.unknownTask << std::endl;
                }
                // We matched this current, change to the next current
                thisCpu.currentTask = otherCpu.currentTask;
            } else {
                // We are either at the end of the trace
                // or the end of the task is not in this other chunk
                // Keep the current unfinished task
            }
        } else {
            if (otherCpu.unknownTask) {
                // This could happen at the beginning of the trace (i.e. the
                // very first sched_switch)

                // TODO: Commented out for the moment to get same results as
                // lttng-analyses
                /*
                uint64_t taskTime = otherCpu.unknownTask->end - this->getStart();
                thisCpu.cpu_ns += taskTime;
                Process &thisProcess = tids[otherCpu.unknownTask->tid];
                thisProcess.cpu_ns += taskTime;
                */
            }
            // We did not have an unfinished task, take the next
            thisCpu.currentTask = otherCpu.currentTask;
        }
    }
}

uint64_t CpuContext::getStart() const
{
    return start;
}

void CpuContext::setStart(const uint64_t &value)
{
    start = value;
}
uint64_t CpuContext::getEnd() const
{
    return end;
}

void CpuContext::setEnd(const uint64_t &value)
{
    end = value;
}
const std::vector<Cpu> &CpuContext::getCpus() const
{
    return cpus;
}
const std::list<Process> &CpuContext::getTids() const
{
    return sortedTids;
}

Cpu &CpuContext::getCpu(unsigned int cpu)
{
    std::vector<Cpu>::iterator iter;
    for (iter = cpus.begin(); iter != cpus.end(); ++iter) {
        if (iter->id == cpu) {
            return *iter;
        }
    }
    cpus.emplace_back(cpu);
    return cpus.back();
}


