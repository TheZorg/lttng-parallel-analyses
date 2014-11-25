#ifndef COUNTANALYSIS_H
#define COUNTANALYSIS_H

#include "traceanalysis.h"

class CountAnalysis : public TraceAnalysis
{
    Q_OBJECT
public:
    CountAnalysis(QObject *parent) : TraceAnalysis(parent) { }

protected:
    virtual void doExecute();
};

class CountWorker : public TraceWorker {
public:
    CountWorker(int id, QString path, bt_iter_pos begin, bt_iter_pos end, bool verbose = false) : TraceWorker(id, path, begin, end, verbose) {}
    CountWorker(CountWorker &&other) : TraceWorker(std::move(other)) {}
};

#endif // COUNTANALYSIS_H
