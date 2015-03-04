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

#ifndef COUNTANALYSIS_H
#define COUNTANALYSIS_H

#include "common/traceanalysis.h"

class CountWorker : public TraceWorker<int> {
public:
    CountWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose = false);
    CountWorker(CountWorker &&other);
    virtual int doMap() const;
    static void doReduce(int &final, const int &intermediate);
};

class CountAnalysis : public TraceAnalysis<CountWorker, int>
{
    Q_OBJECT
public:
    CountAnalysis(QObject *parent) : TraceAnalysis(parent) { }

protected:
    virtual void doEnd(int &data) {(void) data;}
    virtual void printResults(int &data);
    virtual void doExecuteSerial();
    virtual bool isOrderedReduce();

};

#endif // COUNTANALYSIS_H
