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

#endif // COUNTANALYSIS_H
