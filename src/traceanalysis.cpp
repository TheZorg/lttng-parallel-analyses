#include "traceanalysis.h"

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
    doExecute();
    emit finished();
}



