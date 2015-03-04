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

#ifndef TRACEANALYSIS_H
#define TRACEANALYSIS_H

#include "tracewrapper.h"

#include <base/BasicTypes.hpp>
#include <trace/TraceSet.hpp>

#include <QObject>
#include <QTime>
#include <QtConcurrent>

using namespace tibee;
using namespace tibee::trace;

class AbstractTraceAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit AbstractTraceAnalysis(QObject *parent = 0) :
        QObject(parent)
    {
    }

    int getThreads() const
    {
        return threads;
    }
    void setThreads(int value)
    {
        threads = value;
    }

    QString getTracePath() const
    {
        return tracePath;
    }
    void setTracePath(const QString &value)
    {
        tracePath = value;
    }

    bool getVerbose() const
    {
        return verbose;
    }
    void setVerbose(bool value)
    {
        verbose = value;
    }

    bool getIsParallel() const
    {
        return isParallel;
    }
    void setIsParallel(bool value)
    {
        isParallel = value;
    }

    bool getDoBenchmark() const
    {
        return doBenchmark;
    }
    void setDoBenchmark(bool value)
    {
        doBenchmark = value;
    }

signals:
    void finished();

public slots:
    void execute()
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

protected:
    virtual void doExecuteParallel() = 0;
    virtual void doExecuteSerial() = 0;
    virtual bool isOrderedReduce() = 0;

protected:
    int threads;
    bool isParallel;
    QString tracePath;
    bool doBenchmark;
    bool verbose;
};

template <typename WorkerType, typename ReduceResultType>
class TraceAnalysis : public AbstractTraceAnalysis
{
public:
    explicit TraceAnalysis(QObject *parent = 0) :
        AbstractTraceAnalysis(parent)
    {
    }
protected:
    virtual void doEnd(ReduceResultType &data) = 0;
    virtual void printResults(ReduceResultType &data) = 0;
    virtual void doExecuteParallel()
    {
        timestamp_t positions[threads];
        std::vector<WorkerType> workers;
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
        QtConcurrent::ReduceOptions options;
        if (isOrderedReduce()) options = QtConcurrent::OrderedReduce;
        else options = QtConcurrent::UnorderedReduce;
        auto future = QtConcurrent::mappedReduced(workers.begin(), workers.end(),
                                                     &WorkerType::doMap, &WorkerType::doReduce, options);

        auto data = future.result();

        doEnd(data);

        printResults(data);
    }
};

template <typename MapResultType>
class TraceWorker {
public:
    typedef MapResultType MapResult;
    TraceWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose = false) :
        id(id), traceSet(set), beginPos(begin), endPos(end), verbose(verbose)
    {
    }

    // Don't allow copying
    TraceWorker(const TraceWorker &other) = delete;
    TraceWorker& operator=(const TraceWorker &other) & = delete;

    virtual ~TraceWorker()
    {
    }

    // Moving is fine (C++11)
    TraceWorker(TraceWorker &&other) : id(std::move(other.id)), traceSet(other.traceSet),
        beginPos(std::move(other.beginPos)), endPos(std::move(other.endPos)), verbose(std::move(other.verbose))
    {
    }

    // Accessors
    TraceSet &getTraceSet()
    {
        return traceSet;
    }

    const TraceSet &getTraceSet() const
    {
        return traceSet;
    }

    const timestamp_t *getBeginPos() const
    {
        return beginPos;
    }
    void setBeginPos(const timestamp_t *value)
    {
        beginPos = value;
    }

    const timestamp_t *getEndPos() const
    {
        return endPos;
    }
    void setEndPos(const timestamp_t *value)
    {
        endPos = value;
    }

    bool getVerbose() const
    {
        return verbose;
    }
    void setVerbose(bool value)
    {
        verbose = value;
    }

    int getId() const
    {
        return id;
    }
    void setId(int value)
    {
        id = value;
    }

    virtual MapResultType doMap() const = 0;

protected:
    int id;
    TraceSet &traceSet;
    const timestamp_t *beginPos;
    const timestamp_t *endPos;
    bool verbose;
};

#endif // TRACEANALYSIS_H
