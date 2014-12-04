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
