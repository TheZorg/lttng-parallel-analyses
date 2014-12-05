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

using namespace tibee;
using namespace tibee::trace;

class TraceAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit TraceAnalysis(QObject *parent = 0);

    int getThreads() const;
    void setThreads(int value);

    QString getTracePath() const;
    void setTracePath(const QString &value);

    bool getVerbose() const;
    void setVerbose(bool value);

    bool getIsParallel() const;
    void setIsParallel(bool value);

    bool getDoBenchmark() const;
    void setDoBenchmark(bool value);

signals:
    void finished();

public slots:
    void execute();

protected:
    virtual void doExecuteParallel() = 0;
    virtual void doExecuteSerial() = 0;

protected:
    int threads;
    bool isParallel;
    QString tracePath;
    bool doBenchmark;
    bool verbose;
};

class TraceWorker {
public:
    TraceWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose = false);

    // Don't allow copying
    TraceWorker(const TraceWorker &other) = delete;
    TraceWorker& operator=(const TraceWorker &other) & = delete;

    virtual ~TraceWorker();


    // Moving is fine (C++11)
//    TraceWorker(TraceWorker &&other) = default;
    TraceWorker(TraceWorker &&other);

    // Accessors
    TraceSet &getTraceSet();

    const timestamp_t *getBeginPos() const;
    void setBeginPos(const timestamp_t *value);

    const timestamp_t *getEndPos() const;
    void setEndPos(const timestamp_t *value);

    bool getVerbose() const;
    void setVerbose(bool value);

    int getId() const;
    void setId(int value);

protected:
    int id;
    TraceSet &traceSet;
    const timestamp_t *beginPos;
    const timestamp_t *endPos;
    bool verbose;
};

#endif // TRACEANALYSIS_H
