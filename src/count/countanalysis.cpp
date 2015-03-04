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

CountWorker::CountWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose) :
    TraceWorker(id, set, begin, end, verbose)
{
}

int CountWorker::doMap() const {
    const TraceSet &traceSet = getTraceSet();
    TraceSet::Iterator iter = traceSet.between(getBeginPos(), getEndPos());
    TraceSet::Iterator endIter = traceSet.end();

    int count = 0;
    for ((void)iter; iter != endIter; ++iter) {
        count++;
    }

    if (getVerbose()) {
        const timestamp_t *begin = getBeginPos();
        const timestamp_t *end = getEndPos();
        std::string beginString = begin ? std::to_string(*begin) : "START";
        std::string endString = end ? std::to_string(*end) : "END";
        std::cout << "Worker " << getId() << " counted " << count << " events between timestamps "
                  << beginString << " and " << endString << std::endl;
    }

    return count;
}

void CountWorker::doReduce(int &final, const int &intermediate)
{
    final += intermediate;
}

void CountAnalysis::doExecuteSerial() {
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    int count = 0;
    for (const auto &event : set) {
        (void) event;
        count++;
    }

    printResults(count);
}

bool CountAnalysis::isOrderedReduce()
{
    return false;
}

void CountAnalysis::printResults(int &count)
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
