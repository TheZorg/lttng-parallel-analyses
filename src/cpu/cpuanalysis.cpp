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

#include <iostream>
#include <iomanip>
#include <sstream>

#include <QVector>
#include <QHash>

#include <babeltrace/babeltrace.h>
#include <babeltrace/iterator.h>

#include <trace/TraceSet.hpp>

#include <boost/optional.hpp>

void CpuAnalysis::doExecuteParallel()
{

}

void CpuAnalysis::doExecuteSerial()
{
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    // Set begin timestamp
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

event_id_t CpuAnalysis::getEventId(TraceSet &set, std::string eventName)
{
    auto &tracesInfos = set.getTracesInfos();
    for (const auto &traceInfos : tracesInfos) {
        if (traceInfos->getTraceType() == "lttng-kernel") {
            for (const auto &eventNameIdPair : *traceInfos->getEventMap()) {
                if (eventNameIdPair.first == eventName) {
                    return eventNameIdPair.second->getId();
                }
            }
        }
    }
    return -1;
}

void CpuAnalysis::printResults(CpuContext &data)
{
    uint64_t total = data.getEnd() - data.getStart();
    std::string line(80, '-');

    std::cout << line << std::endl;
    std::cout << "Result of count analysis" << std::endl << std::endl;
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
