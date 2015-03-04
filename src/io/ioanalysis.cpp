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

#include "ioanalysis.h"
#include "iocontext.h"
#include "common/utils.h"
#include "common/packetindex.h"

#include <QtConcurrent>
#include <QTemporaryDir>
#include <QUuid>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>

std::vector<std::string> readSyscalls = {"sys_read", "syscall_entry_read",
                              "sys_recvmsg", "syscall_entry_recvmsg",
                              "sys_recvfrom", "syscall_entry_recvfrom",
                              "sys_readv", "syscall_entry_readv"};

std::vector<std::string> writeSyscalls = {"sys_write", "syscall_entry_write",
                               "sys_sendmsg", "syscall_entry_sendmsg",
                               "sys_sendto", "syscall_entry_sendto",
                               "sys_writev", "syscall_entry_writev"};

std::vector<std::string> readWriteSyscalls = {"sys_splice", "syscall_entry_splice",
                                              "sys_sendfile64", "syscall_entry_sendfile64"};

IoWorker::IoWorker(int id, TraceSet &set, timestamp_t *begin, timestamp_t *end, bool verbose) :
    TraceWorker(id, set, begin, end, verbose)
{
}

IoWorker &IoWorker::operator=(IoWorker &&other)
{
    id = std::move(other.id);
    traceSet = std::move(other.traceSet);
    beginPos = std::move(other.beginPos);
    endPos = std::move(other.endPos);
    verbose = std::move(other.verbose);
    return *this;
}

IoWorker::IoWorker(IoWorker &&other) :
    TraceWorker(std::move(other))
{
}

void IoAnalysis::balancedExecute()
{
    std::unordered_map<std::string, TraceSet> traceSets; // Traces will be emplaced in here

    // Create temp dir for split streams
    QDir tmpDir(QDir::tempPath()); // /tmp
    std::cout << tmpDir.absolutePath().toStdString() << std::endl;
    QFileInfo traceDirInfo(tracePath);

    QString perStreamDirPath = traceDirInfo.fileName() + "_per_stream-" + QUuid::createUuid().toString();
    std::cout << perStreamDirPath.toStdString() << std::endl;
    tmpDir.mkdir(perStreamDirPath);
    tmpDir.cd(perStreamDirPath); // /tmp/kernel_per_stream-{...}

    // Iterate through the stream files and symlink in proper dir
    QDir traceDir(tracePath);
    QString metadataPath = traceDir.absoluteFilePath("metadata");
    QFileInfoList fileList = traceDir.entryInfoList(QStringList(), QDir::Files);
    for (int i = 0; i < fileList.size(); i++) {
        QFileInfo fileInfo = fileList.at(i);
        if (fileInfo.fileName() != "metadata") {
            std::cout << fileInfo.fileName().toStdString() << std::endl;
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
    std::vector<IoWorker> workers;
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
        for (unsigned int i = 0; i < positions.size(); i++)
        {
            timestamp_t *begin, *end;
            if (i == 0) {
                begin = nullptr;
            } else {
                begin = &positions[i];
            }
            if (i == positions.size() - 1) {
                end = nullptr;
            } else {
                end = &positions[i+1];
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
    std::sort(workers.begin(), workers.end(), [](const IoWorker &a, const IoWorker &b) {
        if (b.getBeginPos() == NULL) return false;
        if (a.getBeginPos() == NULL) return true;
        return *a.getBeginPos() < *b.getBeginPos();
    });

    // Launch map reduce
    auto ioFuture = QtConcurrent::mappedReduced(workers.begin(), workers.end(),
                                                 &IoWorker::doMap, &IoWorker::doReduce, QtConcurrent::OrderedReduce);

    auto data = ioFuture.result();

    data.handleEnd();

    printResults(data);
}

IoContext IoWorker::doMap() const
{
    const TraceSet &set = getTraceSet();
    TraceSet::Iterator iter = set.between(getBeginPos(), getEndPos());
    TraceSet::Iterator endIter = set.end();

    IoContext data;

    // Get event ids
    std::set<event_id_t> readEventIds {};
    for (const std::string &eventName : readSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readEventIds.insert(id);
    }
    std::set<event_id_t> writeEventIds {};
    for (const std::string &eventName : writeSyscalls) {
        event_id_t id = getEventId(set, eventName);
        writeEventIds.insert(id);
    }
    std::set<event_id_t> readWriteEventIds {};
    for (const std::string &eventName : readWriteSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readWriteEventIds.insert(id);
    }

    event_id_t exitSyscallId = getEventId(set, "exit_syscall");

    // Iterate through events
    uint64_t count = 0;
    for ((void)iter; iter != endIter; ++iter) {
        count++;
        const auto &event = *iter;
        event_id_t id = event.getId();
        if (readEventIds.find(id) != readEventIds.end()) {
            data.handleSysRead(event);
        } else if (writeEventIds.find(id) != writeEventIds.end()) {
            data.handleSysWrite(event);
        } else if (readWriteEventIds.find(id) != readWriteEventIds.end()) {
            data.handleSysReadWrite(event);
        } else if (id == exitSyscallId) {
            data.handleExitSyscall(event);
        }
    }

    if (getVerbose()) {
        const timestamp_t *begin = getBeginPos();
        const timestamp_t *end = getEndPos();
        std::string beginString = begin ? std::to_string(*begin) : "START";
        std::string endString = end ? std::to_string(*end) : "END";
        std::cout << "Worker " << getId() << " processed " << count << " events between timestamps "
                  << beginString << " and " << endString << std::endl;
    }

    data.handleEnd();

    return data;
}

void IoWorker::doReduce(IoContext &final, const IoContext &intermediate)
{
    final.merge(intermediate);
}

bool IoAnalysis::isOrderedReduce()
{
    return true;
}

void IoAnalysis::doExecuteSerial()
{
    TraceSet set;
    set.addTrace(this->tracePath.toStdString());

    IoContext data;

    // Get event ids
    std::set<event_id_t> readEventIds {};
    for (const std::string &eventName : readSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readEventIds.insert(id);
    }
    std::set<event_id_t> writeEventIds {};
    for (const std::string &eventName : writeSyscalls) {
        event_id_t id = getEventId(set, eventName);
        writeEventIds.insert(id);
    }
    std::set<event_id_t> readWriteEventIds {};
    for (const std::string &eventName : readWriteSyscalls) {
        event_id_t id = getEventId(set, eventName);
        readWriteEventIds.insert(id);
    }

    event_id_t exitSyscallId = getEventId(set, "exit_syscall");

    // Iterate through events
    for (const auto &event : set) {
        event_id_t id = event.getId();
        if (readEventIds.find(id) != readEventIds.end()) {
            data.handleSysRead(event);
        } else if (writeEventIds.find(id) != writeEventIds.end()) {
            data.handleSysWrite(event);
        } else if (readWriteEventIds.find(id) != readWriteEventIds.end()) {
            data.handleSysReadWrite(event);
        } else if (id == exitSyscallId) {
            data.handleExitSyscall(event);
        }
    }

    data.handleEnd();

    printResults(data);
}

void IoAnalysis::printResults(IoContext &data)
{
    std::string line(80, '-');
    int max = 10;
    int count = 0;
    int colWidth = 30;

    std::cout << line << std::endl;
    std::cout << "Result of I/O analysis" << std::endl << std::endl;
    std::cout << "Syscall I/O Read" << std::endl << std::endl;
    std::cout << std::setw(colWidth) << std::left << "Process" <<
                 std::setw(colWidth) << std::left << "Size" << std::endl;

    for (const IoProcess &process : data.getTidsByRead()) {
        if (++count > max) {
            break;
        }
        std::stringstream ss;
        ss << process.comm << " (" << process.tid << ")";
        std::cout << std::setw(colWidth) << std::left << ss.str() <<
                     std::setw(colWidth) << std::left << convertSize(process.readBytes) << std::endl;
    }
    std::cout << line << std::endl;
    std::cout << "Syscall I/O Write" << std::endl << std::endl;
    std::cout << std::setw(colWidth) << std::left << "Process" <<
                 std::setw(colWidth) << std::left << "Size" << std::endl;
    count = 0;
    for (const IoProcess &process : data.getTidsByWrite()) {
        if (++count > max) {
            break;
        }
        std::stringstream ss;
        ss << process.comm << " (" << process.tid << ")";
        std::cout << std::setw(colWidth) << std::left << ss.str() <<
                     std::setw(colWidth) << std::left << convertSize(process.writeBytes) << std::endl;
    }
}

void IoAnalysis::doEnd(IoContext &data)
{
    data.handleEnd();
}
