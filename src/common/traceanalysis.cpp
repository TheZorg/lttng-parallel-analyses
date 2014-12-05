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

#include "traceanalysis.h"

#include <QTime>

using namespace tibee;

TraceAnalysis::TraceAnalysis(QObject *parent) :
    QObject(parent)
{
}

int TraceAnalysis::getThreads() const
{
    return threads;
}

void TraceAnalysis::setThreads(int value)
{
    threads = value;
}
QString TraceAnalysis::getTracePath() const
{
    return tracePath;
}

void TraceAnalysis::setTracePath(const QString &value)
{
    tracePath = value;
}
bool TraceAnalysis::getVerbose() const
{
    return verbose;
}

void TraceAnalysis::setVerbose(bool value)
{
    verbose = value;
}

void TraceAnalysis::execute()
{
    QTime timer;
    if (doBenchmark) {
        timer.start();
    }
    if (isParallel) {
        doExecuteParallel();
    } else {
        doExecuteSerial();
    }
    if (doBenchmark) {
        int milliseconds = timer.elapsed();
        std::cout << "Analysis time : " << milliseconds << "ms." << std::endl;
    }
    emit finished();
}
bool TraceAnalysis::getDoBenchmark() const
{
    return doBenchmark;
}

void TraceAnalysis::setDoBenchmark(bool value)
{
    doBenchmark = value;
}

bool TraceAnalysis::getIsParallel() const
{
    return isParallel;
}

void TraceAnalysis::setIsParallel(bool value)
{
    isParallel = value;
}



TraceWorker::TraceWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose) :
    id(id), traceSet(set), beginPos(begin), endPos(end), verbose(verbose)
{
}

TraceWorker::~TraceWorker()
{
}

TraceWorker::TraceWorker(TraceWorker &&other) : id(std::move(other.id)), traceSet(other.traceSet),
    beginPos(std::move(other.beginPos)), endPos(std::move(other.endPos)), verbose(std::move(other.verbose))
{
}

TraceSet& TraceWorker::getTraceSet()
{
    return traceSet;
}
const timestamp_t *TraceWorker::getBeginPos() const
{
    return beginPos;
}

void TraceWorker::setBeginPos(const timestamp_t *value)
{
    beginPos = value;
}
const timestamp_t *TraceWorker::getEndPos() const
{
    return endPos;
}

void TraceWorker::setEndPos(const timestamp_t *value)
{
    endPos = value;
}
bool TraceWorker::getVerbose() const
{
    return verbose;
}

void TraceWorker::setVerbose(bool value)
{
    verbose = value;
}
int TraceWorker::getId() const
{
    return id;
}

void TraceWorker::setId(int value)
{
    id = value;
}




