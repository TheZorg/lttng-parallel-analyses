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

#ifndef CPUANALYSIS_H
#define CPUANALYSIS_H

#include "common/traceanalysis.h"
#include "cpucontext.h"

class CpuWorker : public TraceWorker<CpuContext>
{
public:

    CpuWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose = false);
    CpuWorker(CpuWorker &&other);

    CpuContext &getData();

    virtual CpuContext doMap() const;
    static void doReduce(CpuContext &final, const CpuContext &intermediate);
};

class CpuAnalysis : public TraceAnalysis<CpuWorker, CpuContext>
{
    Q_OBJECT
public:
    CpuAnalysis(QObject *parent) : TraceAnalysis(parent) { }

protected:
    virtual bool isOrderedReduce();
    virtual void doExecuteSerial();
    virtual void printResults(CpuContext &data);
    virtual void doEnd(CpuContext &data);
};

#endif // CPUANALYSIS_H
