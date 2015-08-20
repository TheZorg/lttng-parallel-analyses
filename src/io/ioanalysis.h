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

#ifndef IOANALYSIS_H
#define IOANALYSIS_H

#include "common/traceanalysis.h"
#include "iocontext.h"

class IoWorker : public TraceWorker<IoContext>
{
public:
    IoWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose = false);
    IoWorker(IoWorker &other) = delete;
    IoWorker &operator =(const IoWorker &other) = delete;

    IoWorker(IoWorker &&other) : TraceWorker<IoContext>(std::move(other)) {}
    IoWorker &operator =(IoWorker &&other)
    {
        TraceWorker<IoContext>::operator =(std::move(other));
        return *this;
    }

    virtual IoContext doMap() const;
    static void doReduce(IoContext &final, const IoContext &intermediate);

};

class IoAnalysis : public TraceAnalysis<IoWorker, IoContext>
{
    Q_OBJECT
public:
    IoAnalysis(QObject *parent) : TraceAnalysis(parent) {}

protected:
    virtual bool isOrderedReduce();
    virtual void doExecuteSerial();
    virtual void printResults(IoContext &data);
    virtual void doEnd(IoContext &data);
//    virtual void doExecuteParallelBalanced()
//    {
//        std::cerr << "Balanced analysis not yet supported." << std::endl;
//    }
};

#endif // IOANALYSIS_H
