#ifndef TRACEANALYSIS_H
#define TRACEANALYSIS_H

#include "tracewrapper.h"

#include <babeltrace/babeltrace.h>

#include <QObject>

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

signals:
    void finished();

public slots:
    void execute();

protected:
    virtual void doExecute() = 0;

protected:
    int threads;
    QString tracePath;
    bool verbose;
};

class TraceWorker {
public:
    TraceWorker(int id, QString path, bt_iter_pos begin, bt_iter_pos end, bool verbose = false) :
        id(id), wrapper(path), beginPos(begin), endPos(end), verbose(verbose) {}

    // Don't allow copying
    TraceWorker(const TraceWorker &other) = delete;
    TraceWorker& operator=(const TraceWorker &other) & = delete;


    // Moving is fine (C++11)
    TraceWorker(TraceWorker &&other) = default;
//    TraceWorker(TraceWorker &&other) : id(std::move(id)), wrapper(std::move(other.wrapper)),
//        beginPos(std::move(other.beginPos)), endPos(std::move(other.endPos)), verbose(std::move(verbose)) {}

    // Accessors
    TraceWrapper &getWrapper();

    const bt_iter_pos &getBeginPos() const;
    void setBeginPos(const bt_iter_pos &value);

    const bt_iter_pos &getEndPos() const;
    void setEndPos(const bt_iter_pos &value);

    bool getVerbose() const;
    void setVerbose(bool value);

    int getId() const;
    void setId(int value);

protected:
    int id;
    TraceWrapper wrapper;
    bt_iter_pos beginPos;
    bt_iter_pos endPos;
    bool verbose;
};

#endif // TRACEANALYSIS_H
