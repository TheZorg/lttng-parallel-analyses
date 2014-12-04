#ifndef COUNTANALYSIS_H
#define COUNTANALYSIS_H

#include "traceanalysis.h"

class CountAnalysis : public TraceAnalysis
{
    Q_OBJECT
public:
    CountAnalysis(QObject *parent) : TraceAnalysis(parent) { }

protected:
    virtual void doExecuteParallel();
    virtual void doExecuteSerial();

private:
    void printCount(int count);
};

class CountWorker : public TraceWorker {
public:
    CountWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose = false);
    CountWorker(CountWorker &&other);
};

#endif // COUNTANALYSIS_H
