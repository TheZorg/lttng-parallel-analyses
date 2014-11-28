#include "traceanalysis.h"

using namespace tibee;

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
    if (isParallel) {
        doExecuteParallel();
    } else {
        doExecuteSerial();
    }
    emit finished();
}
bool TraceAnalysis::getIsParallel() const
{
    return isParallel;
}

void TraceAnalysis::setIsParallel(bool value)
{
    isParallel = value;
}



TraceWorker::TraceWorker(int id, QString path, timestamp_t *begin, timestamp_t *end, bool verbose) :
    id(id), beginPos(begin), endPos(end), verbose(verbose)
{
    traceSet.addTrace(path.toStdString());
    std::cout << "Constructing " << id << std::endl;
}

TraceWorker::~TraceWorker()
{
    std::cout << "Destroying " << id << std::endl;
}

TraceWorker::TraceWorker(TraceWorker &&other) : id(std::move(other.id)), traceSet(std::move(other.traceSet)),
    beginPos(std::move(other.beginPos)), endPos(std::move(other.endPos)), verbose(std::move(other.verbose))
{
    std::cout << "Moving " << id << std::endl;
}

TraceSet& TraceWorker::getTraceSet()
{
    return traceSet;
}
const timestamp_t *TraceWorker::getBeginPos() const
{
    return beginPos;
}

void TraceWorker::setBeginPos(const timestamp_t *value)
{
    beginPos = value;
}
const timestamp_t *TraceWorker::getEndPos() const
{
    return endPos;
}

void TraceWorker::setEndPos(const timestamp_t *value)
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




