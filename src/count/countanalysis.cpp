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

#include "countanalysis.h"

#include <QList>
#include <QString>
#include <QtConcurrent/QtConcurrent>

#include <iostream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#include <trace/TraceSet.hpp>
#include <base/BasicTypes.hpp>

using namespace tibee;
using namespace tibee::trace;

static void doSum(int &finalResult, const int &intermediate);
static int doCount(CountWorker &worker);

void CountAnalysis::doExecuteParallel() {
    timestamp_t positions[threads];
    std::vector<CountWorker> workers;
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
    QFuture<int> countFuture = QtConcurrent::mappedReduced(workers.begin(), workers.end(), doCount, doSum);

    int count = countFuture.result();

    printCount(count);
}

int doCount(CountWorker &worker) {
    TraceSet &traceSet = worker.getTraceSet();
    TraceSet::Iterator iter = traceSet.between(worker.getBeginPos(), worker.getEndPos());
    TraceSet::Iterator endIter = traceSet.end();

    int count = 0;
    for ((void)iter; iter != endIter; ++iter) {
        count++;
    }

    if (worker.getVerbose()) {
        const timestamp_t *begin = worker.getBeginPos();
        const timestamp_t *end = worker.getEndPos();
        std::string beginString = begin ? std::to_string(*begin) : "START";
        std::string endString = end ? std::to_string(*end) : "END";
        std::cout << "Worker " << worker.getId() << " counted " << count << " events between timestamps "
                  << beginString << " and " << endString << std::endl;
    }

    return count;
}

void CountAnalysis::doExecuteSerial() {
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    int count = 0;
    for (const auto &event : set) {
        (void) event;
        count++;
    }

    printCount(count);
}

void CountAnalysis::printCount(int count)
{
    std::string line(80, '-');

    // Format the count with thousand seperators
    std::stringstream countss;
    countss.imbue(std::locale(""));
    countss << std::fixed << count;

    std::cout << line << std::endl;
    std::cout << "Result of count analysis" << std::endl << std::endl;
    std::cout << std::setw(20) << std::left << "Number of events" << countss.str() << std::endl;
    std::cout << line << std::endl;
}

void doSum(int &finalResult, const int &intermediate)
{
    finalResult += intermediate;
}

CountWorker::CountWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose) :
    TraceWorker(id, set, begin, end, verbose)
{
}

CountWorker::CountWorker(CountWorker &&other) :
    TraceWorker(std::move(other))
{
}
