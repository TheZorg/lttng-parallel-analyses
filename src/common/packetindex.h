/* Copyright (c) 2015 Fabien Reumont-Locke <fabien.reumont-locke@polymtl.ca>
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

#ifndef PACKETINDEX_H
#define PACKETINDEX_H

#include <cstdint>
#include <string>
#include <vector>

#include <sys/types.h>

#include <trace/TraceSet.hpp>

using namespace tibee::trace;

/*
 * Header at the beginning of each index file.
 * All integer fields are stored in big endian.
 */
struct CtfPacketIndexFileHeader {
    uint32_t magic;
    uint32_t index_major;
    uint32_t index_minor;
    /* struct packet_index_len, in bytes */
    uint32_t packet_index_len;
} __attribute__((__packed__));

/*
 * Packet index generated for each trace packet store in a trace file.
 * All integer fields are stored in big endian.
 */
struct CtfPacketIndex {
    uint64_t offset;		/* offset of the packet in the file, in bytes */
    uint64_t packetSize;		/* packet size, in bits */
    uint64_t contentSize;		/* content size, in bits */
    uint64_t timestampBegin;
    uint64_t timestampEnd;
    uint64_t eventsDiscarded;
    uint64_t streamId;
} __attribute__((__packed__));

struct PacketIndexTime {
    uint64_t timestampBegin;
    uint64_t timestampEnd;
};

struct PacketHeader {
    off_t offset;		/* offset of the packet in the file, in bytes */
    int64_t dataOffset;	/* offset of data within the packet, in bits */
    uint64_t packetSize;	/* packet size, in bits */
    uint64_t contentSize;	/* content size, in bits */
    uint64_t eventsDiscarded;
    uint64_t eventsDiscardedLen;	/* length of the field, in bits */
    struct PacketIndexTime tsCycles;	/* timestamp in cycles */
    struct PacketIndexTime tsReal;	/* realtime timestamp */
};

typedef std::vector<PacketHeader> Indices;
class PacketIndex
{
private:
    int streamId;
    CtfPacketIndexFileHeader header;
    std::vector<PacketHeader> indices;
public:
    PacketIndex(std::string packetIndexPath, const TraceSet &trace);
    const std::vector<PacketHeader> &getPacketIndex() const;
    int getStreamId() const;
};

#endif // PACKETINDEX_H
