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


TraceWrapper& TraceWorker::getWrapper()
{
    return wrapper;
}
const bt_iter_pos &TraceWorker::getBeginPos() const
{
    return beginPos;
}

void TraceWorker::setBeginPos(const bt_iter_pos &value)
{
    beginPos = value;
}
const bt_iter_pos &TraceWorker::getEndPos() const
{
    return endPos;
}

void TraceWorker::setEndPos(const bt_iter_pos &value)
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




