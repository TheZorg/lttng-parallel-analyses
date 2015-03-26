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

#ifndef TRACEANALYSIS_H
#define TRACEANALYSIS_H

#include <base/BasicTypes.hpp>
#include <trace/TraceSet.hpp>

#include <QObject>
#include <QTime>
#include <QtConcurrent>

#include <functional>

#include "common/packetindex.h"
#include "common/traceanalysis.h"

using namespace tibee;
using namespace tibee::trace;

class AbstractTraceAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit AbstractTraceAnalysis(QObject *parent = 0) :
        QObject(parent)
    {
    }

    int getThreads() const
    {
        return threads;
    }
    void setThreads(int value)
    {
        threads = value;
    }

    QString getTracePath() const
    {
        return tracePath;
    }
    void setTracePath(const QString &value)
    {
        tracePath = value;
    }

    bool getVerbose() const
    {
        return verbose;
    }
    void setVerbose(bool value)
    {
        verbose = value;
    }

    bool getIsParallel() const
    {
        return isParallel;
    }
    void setIsParallel(bool value)
    {
        isParallel = value;
    }

    bool getDoBenchmark() const
    {
        return doBenchmark;
    }
    void setDoBenchmark(bool value)
    {
        doBenchmark = value;
    }

    bool getBalanced() const
    {
        return balanced;
    }
    void setBalanced(bool value)
    {
        balanced = value;
    }

signals:
    void finished();

public slots:
    void execute()
    {
        QTime timer;
        if (doBenchmark) {
            timer.start();
        }
        if (isParallel) {
            doExecuteParallel();
        } else {
            doExecuteSerial();
        }
        if (doBenchmark) {
            int milliseconds = timer.elapsed();
            std::cout << "Analysis time (ms) : " << milliseconds << std::endl;
        }
        emit finished();
    }

protected:
    virtual void doExecuteParallel() = 0;
    virtual void doExecuteSerial() = 0;
    virtual bool isOrderedReduce() = 0;

protected:
    int threads;
    bool isParallel;
    QString tracePath;
    bool doBenchmark;
    bool verbose;
    bool balanced;
};

template <typename WorkerType, typename ReduceResultType>
class TraceAnalysis : public AbstractTraceAnalysis
{
public:
    explicit TraceAnalysis(QObject *parent = 0) :
        AbstractTraceAnalysis(parent)
    {
    }
protected:
    virtual void doEnd(ReduceResultType &data) = 0;
    virtual void printResults(ReduceResultType &data) = 0;
    virtual void doExecuteParallel()
    {
        QThreadPool::globalInstance()->setMaxThreadCount(this->threads);
        if (!balanced) {
            doExecuteParallelUnbalanced();
        } else {
            doExecuteParallelBalanced();
        }
    }

    virtual void doExecuteParallelBalanced()
    {
        std::unordered_map<std::string, TraceSet> traceSets; // Traces will be emplaced in here

        // Create temp dir for split streams
        QDir tmpDir(QDir::tempPath()); // /tmp
        QFileInfo traceDirInfo(tracePath);

        QString perStreamDirPath = traceDirInfo.fileName() + "_per_stream-" + QUuid::createUuid().toString();
        tmpDir.mkdir(perStreamDirPath);
        tmpDir.cd(perStreamDirPath); // /tmp/kernel_per_stream-{...}

        // Iterate through the stream files and symlink in proper dir
        QDir traceDir(tracePath);
        QString metadataPath = traceDir.absoluteFilePath("metadata");
        QFileInfoList fileList = traceDir.entryInfoList(QStringList(), QDir::Files);
        for (int i = 0; i < fileList.size(); i++) {
            QFileInfo fileInfo = fileList.at(i);
            if (fileInfo.fileName() != "metadata") {
                // Make a dir for the stream
                QString streamDirPath = fileInfo.fileName() + ".d";
                tmpDir.mkdir(streamDirPath);
                tmpDir.cd(streamDirPath); // /tmp/kernel_per_stream-{...}/channel0_*.d

                // Make a symlink to the stream
                QFile::link(fileInfo.absoluteFilePath(), tmpDir.absoluteFilePath(fileInfo.fileName()));

                // Make a symlink to the metadata
                QFile::link(metadataPath, tmpDir.absoluteFilePath("metadata"));

                // Index
                tmpDir.mkdir("index");
                tmpDir.cd("index"); // /tmp/kernel_per_stream-{...}/channel0_*.d/index
                traceDir.cd("index");
                QFile::link(traceDir.absoluteFilePath(fileInfo.fileName() + ".idx"), tmpDir.absoluteFilePath(fileInfo.fileName() + ".idx"));
                traceDir.cdUp();
                tmpDir.cdUp(); // /tmp/kernel_per_stream-{...}/channel0_*.d

                // Add traceset
                auto ret = traceSets.emplace(fileInfo.fileName().toStdString(), TraceSet());
                // emplace returns a pair with first = iterator to the inserted pair
                ret.first->second.addTrace(tmpDir.absolutePath().toStdString());

                // Go back up
                tmpDir.cdUp(); // /tmp/kernel_per_stream-{...}
            }
        }

        // Parse packet indices
        std::vector<WorkerType> workers;
        std::unordered_map<std::string, std::vector<timestamp_t>> positionsPerTrace;
        traceDir.cd("index");
        fileList = traceDir.entryInfoList(QStringList(), QDir::Files);
        for (int i = 0; i < fileList.size(); i++) {
            QFileInfo fileInfo = fileList.at(i);
            std::string name = fileInfo.baseName().toStdString();
            TraceSet &trace = traceSets.at(name);
            std::vector<timestamp_t> &positions = positionsPerTrace[name];
            PacketIndex index(fileInfo.absoluteFilePath().toStdString(), trace);
            const std::vector<PacketHeader> &indices = index.getPacketIndex();

            // Get total size for all the packets
            uint64_t size = 0;
            for (const PacketHeader &header : indices) {
                size += header.contentSize;
            }

            // Try to split packets "evenly"
            uint64_t maxPacketSize = size/indices.size();
            if (this->verbose) {
                std::cout << "Num packets for stream " << index.getStreamId()
                          << " : " << indices.size() << std::endl;
            }

            // Accumulator for packet size
            uint64_t acc = 0;

            // Iterate through packets, accumulating until we have a big enough chunk
            // NOTE: disregard last packet, since the BT_SEEK_LAST will take care of it
            int numChunks = 0;
            for (unsigned int i = 0; i < indices.size() - 1; i++) {
                PacketHeader header = indices[i];
                acc += header.contentSize;
                if (acc >= maxPacketSize) {
                    // Our chunk is big enough, add the timestamp to our positions
                    numChunks++;
                    acc = 0;
                    positions.push_back(header.tsReal.timestampEnd);
                }
            }

            // Build the params list
            for (unsigned int i = 0; i <= positions.size(); i++)
            {
                timestamp_t *begin, *end;
                if (i == 0) {
                    begin = nullptr;
                } else {
                    begin = &positions[i - 1];
                }
                if (i == positions.size()) {
                    end = nullptr;
                } else {
                    end = &positions[i];
                }
                workers.emplace_back(i, trace, begin, end, verbose);
            }

            if (this->verbose) {
                std::cout << "Num chunks for stream " << index.getStreamId()
                          << " : " << numChunks << std::endl;
            }

        }
        traceDir.cdUp();

        // Sort by begin time
        std::sort(workers.begin(), workers.end(), [](const WorkerType &a, const WorkerType &b) {
            if (b.getBeginPos() == NULL) return false;
            if (a.getBeginPos() == NULL) return true;
            return *a.getBeginPos() < *b.getBeginPos();
        });

        // Launch map reduce
        QtConcurrent::ReduceOptions options;
        if (isOrderedReduce()) options = QtConcurrent::OrderedReduce;
        else options = QtConcurrent::UnorderedReduce;
        auto future = QtConcurrent::mappedReduced(workers.begin(), workers.end(),
                                                     &WorkerType::doMap, &WorkerType::doReduce, options);

        auto data = future.result();

        doEnd(data);

        printResults(data);

        // Remove the temporary directory
        tmpDir.removeRecursively();
    }

    virtual void doExecuteParallelUnbalanced()
    {
        timestamp_t positions[threads];
        std::vector<WorkerType> workers;
        timestamp_t begin, end;

        // Open a trace to get the begin/end timestamps
        TraceSet set;
        set.addTrace(this->tracePath.toStdString());

        // Get begin timestamp
        begin = set.getBegin();

        // Get end timestamp
        end = set.getEnd();

        // Calculate begin/end timestamp pairs for each chunk
        timestamp_t step = (end - begin)/threads;

        for (int i = 0; i < threads; i++)
        {
            positions[i] = begin + (i*step);
        }

        // Build the params list
        for (int i = 0; i < threads; i++)
        {
            timestamp_t *begin, *end;
            if (i == 0) {
                begin = nullptr;
            } else {
                begin = &positions[i];
            }
            if (i == threads - 1) {
                end = nullptr;
            } else {
                end = &positions[i+1];
            }
            workers.emplace_back(i, set, begin, end, verbose);
        }

        // Launch map reduce
        QtConcurrent::ReduceOptions options;
        if (isOrderedReduce()) options = QtConcurrent::OrderedReduce;
        else options = QtConcurrent::UnorderedReduce;
        auto future = QtConcurrent::mappedReduced(workers.begin(), workers.end(),
                                                     &WorkerType::doMap, &WorkerType::doReduce, options);

        auto data = future.result();

        doEnd(data);

        printResults(data);
    }
};

template <typename MapResultType>
class TraceWorker {
public:
    typedef MapResultType MapResult;
    TraceWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose = false) :
        id(id), traceSet(set), verbose(verbose)
    {
        if (begin != NULL) {
            beginPosVal = *begin+1; // Fixes overlap problem
            beginPos = &beginPosVal;
        } else {
            beginPos = NULL;
        }
        if (end != NULL) {
            endPosVal = *end;
            endPos = &endPosVal;
        } else {
            endPos = NULL;
        }
    }

    // Don't allow copying
    TraceWorker(const TraceWorker &other) = delete;
    TraceWorker& operator=(const TraceWorker &other) = delete;

    virtual ~TraceWorker()
    {
    }

    // Moving is fine (C++11)
    TraceWorker(TraceWorker &&other) : id(std::move(other.id)), traceSet(other.traceSet),
        beginPos(std::move(other.beginPos)), endPos(std::move(other.endPos)), verbose(std::move(other.verbose))
    {
        if (other.beginPos != NULL) {
            beginPosVal = *other.beginPos;
            beginPos = &beginPosVal;
        } else {
            beginPos = NULL;
        }
        if (other.endPos != NULL) {
            endPosVal = *other.endPos;
            endPos = &endPosVal;
        } else {
            endPos = NULL;
        }
        other.beginPos = NULL;
        other.endPos = NULL;
    }

    TraceWorker &operator=(TraceWorker &&other)
    {
        if (this != &other) {
            id = std::move(other.id);
            traceSet = std::move(other.traceSet);
            verbose = std::move(other.verbose);
            if (other.beginPos != NULL) {
                beginPosVal = *other.beginPos;
                beginPos = &beginPosVal;
            } else {
                beginPos = NULL;
            }
            if (other.endPos != NULL) {
                endPosVal = *other.endPos;
                endPos = &endPosVal;
            } else {
                endPos = NULL;
            }
            other.beginPos = NULL;
            other.endPos = NULL;
        }
        return *this;
    }

    // Accessors
    TraceSet &getTraceSet()
    {
        return traceSet;
    }

    const TraceSet &getTraceSet() const
    {
        return traceSet;
    }

    const timestamp_t *getBeginPos() const
    {
        return beginPos;
    }
    void setBeginPos(const timestamp_t *value)
    {
        beginPos = value;
    }

    const timestamp_t *getEndPos() const
    {
        return endPos;
    }
    void setEndPos(const timestamp_t *value)
    {
        endPos = value;
    }

    bool getVerbose() const
    {
        return verbose;
    }
    void setVerbose(bool value)
    {
        verbose = value;
    }

    int getId() const
    {
        return id;
    }
    void setId(int value)
    {
        id = value;
    }

    virtual MapResultType doMap() const = 0;

protected:
    int id;
    std::reference_wrapper<TraceSet> traceSet;
    timestamp_t beginPosVal;
    timestamp_t endPosVal;
    const timestamp_t *beginPos;
    const timestamp_t *endPos;
    bool verbose;
};

#endif // TRACEANALYSIS_H
