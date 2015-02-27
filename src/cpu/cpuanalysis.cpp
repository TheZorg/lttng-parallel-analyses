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

#include "cpuanalysis.h"
#include "cpucontext.h"
#include "common/utils.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include <QVector>
#include <QHash>
#include <QtConcurrent>

#include <babeltrace/babeltrace.h>
#include <babeltrace/iterator.h>

#include <trace/TraceSet.hpp>

#include <boost/optional.hpp>

CpuContext doMap(CpuWorker &worker);
void doReduce(CpuContext &final, const CpuContext &intermediate);

void CpuAnalysis::doExecuteParallel()
{
    timestamp_t positions[threads];
    std::vector<CpuWorker> workers;
    timestamp_t begin, end;

    // Open a trace to get the begin/end timestamps
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    // Get begin timestamp
    begin = set.getBegin();

    // Get end timestamp
    end = set.getEnd();

    // Calculate begin/end timestamp pairs for each chunk
    timestamp_t step = (end - begin)/threads;

    for (int i = 0; i < threads; i++)
    {
        positions[i] = begin + (i*step);
    }

    // Build the params list
    for (int i = 0; i < threads; i++)
    {
        timestamp_t *begin, *end;
        if (i == 0) {
            begin = nullptr;
        } else {
            begin = &positions[i];
        }
        if (i == threads - 1) {
            end = nullptr;
        } else {
            end = &positions[i+1];
        }
        workers.emplace_back(i, set, begin, end, verbose);
    }

    // Launch map reduce
    auto cpuFuture = QtConcurrent::mappedReduced(workers.begin(), workers.end(),
                                                 doMap, doReduce, QtConcurrent::OrderedReduce);

    auto data = cpuFuture.result();

    data.handleEnd();

    printResults(data);
}

CpuContext doMap(CpuWorker &worker)
{
    TraceSet &traceSet = worker.getTraceSet();
    TraceSet::Iterator iter = traceSet.between(worker.getBeginPos(), worker.getEndPos());
    TraceSet::Iterator endIter = traceSet.end();

    // Set begin and end timestamps
    const timestamp_t *begin = worker.getBeginPos();
    const timestamp_t *end = worker.getEndPos();
    CpuContext &data = worker.getData();
    data.setStart(begin ? *begin : traceSet.getBegin());
    data.setEnd(end ? *end : traceSet.getEnd());

    // Get sched_switch event id
    event_id_t schedSwitchId = getEventId(traceSet, "sched_switch");

    if (schedSwitchId < 0) {
        std::cerr << "The trace is missing sched_switch events." << std::endl;
        return data;
    }
    uint64_t count = 0;
    for ((void)iter; iter != endIter; ++iter) {
        count++;
        const auto &event = *iter;
        event_id_t id = event.getId();
        if (id == schedSwitchId) {
            data.handleSchedSwitch(event);
        }
    }

    if (worker.getVerbose()) {
        const timestamp_t *begin = worker.getBeginPos();
        const timestamp_t *end = worker.getEndPos();
        std::string beginString = begin ? std::to_string(*begin) : "START";
        std::string endString = end ? std::to_string(*end) : "END";
        std::cout << "Worker " << worker.getId() << " processed " << count << " events between timestamps "
                  << beginString << " and " << endString << std::endl;
    }

    data.handleEnd();

    return data;
}

void doReduce(CpuContext &final, const CpuContext &intermediate)
{
    final.merge(intermediate);
}

void CpuAnalysis::doExecuteSerial()
{
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    // Set begin and end timestamps
    CpuContext data;
    data.setStart(set.getBegin());
    data.setEnd(set.getEnd());

    // Get sched_switch event id
    event_id_t schedSwitchId = getEventId(set, "sched_switch");

    if (schedSwitchId < 0) {
        std::cerr << "The trace is missing sched_switch events." << std::endl;
        return;
    }

    // Iterate through sched_switch events
    for (const auto &event : set) {
        event_id_t id = event.getId();
        if (id == schedSwitchId) {
            data.handleSchedSwitch(event);
        }
    }

    data.handleEnd();

    printResults(data);
}

void CpuAnalysis::printResults(CpuContext &data)
{
    uint64_t total = data.getEnd() - data.getStart();
    std::string line(80, '-');

    std::cout << line << std::endl;
    std::cout << "Result of cpu analysis" << std::endl << std::endl;
    std::cout << std::setw(20) << std::left << "CPU" << "Percentage time" << std::endl;

    // Print out CPU
    for (const Cpu &cpu : data.getCpus()) {
        std::stringstream ss;
        ss << "CPU " << cpu.id;
        double pc = ((double)(cpu.cpu_ns * 100))/((double)total);
        std::cout << std::setw(20) << std::left << ss.str()
                  << std::setprecision(2) << std::fixed << pc << std::endl;
    }
    std::cout << line << std::endl;
    std::cout << std::setw(30) << std::left << "Process" << "Percentage time" << std::endl;

    // Print out top 10 processes
    int max = 10;
    int count = 0;
    for (const Process &process : data.getTids()) {
        if (++count > max) {
            break;
        }
        std::stringstream ss;
        ss << process.comm << " (" << process.tid << ")";
        double pc = ((double)(process.cpu_ns * 100))/((double)total);
        std::cout << std::setw(30) << std::left << ss.str()
                  << std::setprecision(2) << std::fixed << pc << std::endl;
    }
}


CpuWorker::CpuWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose) :
    TraceWorker(id, set, begin, end, verbose), data()
{
}

CpuWorker::CpuWorker(CpuWorker &&other) :
    TraceWorker(std::move(other)), data(std::move(other.data))
{
}

CpuContext &CpuWorker::getData()
{
    return data;
}
