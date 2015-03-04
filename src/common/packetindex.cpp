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

#include "packetindex.h"

#include <cstdio>
#include <cstring>
#include <iostream>

#include <QDir>

#include <endian.h>

#define CTF_INDEX_MAGIC 0xC1F1DCC1
#define CTF_INDEX_MAJOR 1
#define CTF_INDEX_MINOR 0

using namespace tibee::trace;

static inline
uint64_t clock_cycles_to_ns(const ClockInfos &clock, uint64_t cycles)
{
    if (clock.freq == 1000000000ULL) {
        /* 1GHZ freq, no need to scale cycles value */
        return cycles;
    } else {
        return (double) cycles * 1000000000.0
                / (double) clock.freq;
    }
}

static inline
uint64_t clock_offset_ns(const ClockInfos &clock)
{
    return clock.offset_s * 1000000000ULL
            + clock_cycles_to_ns(clock, clock.offset);
}

static inline
uint64_t ctf_get_real_timestamp(const ClockInfos &clock, uint64_t timestamp)
{
    uint64_t ts_nsec;
    uint64_t tc_offset;

    tc_offset = clock_offset_ns(clock);

    ts_nsec = clock_cycles_to_ns(clock, timestamp);
    ts_nsec += tc_offset;	/* Add offset */
    return ts_nsec;
}

PacketIndex::PacketIndex(std::string packetIndexPath, const TraceSet &trace)
{
    FILE *fp = fopen(packetIndexPath.c_str(), "r");
    if (!fp) {
        std::cerr << "Error: could not open index file" << std::endl;
        fclose(fp);
        return;
    }

    size_t len = fread(&header, sizeof(header), 1, fp);
    if (len != 1) {
        std::cerr << "Error: read index file header" << std::endl;
        fclose(fp);
        return;
    }

    if (be32toh(header.magic) != CTF_INDEX_MAGIC) {
        std::cerr << "Error: wrong index magic" << std::endl;
        fclose(fp);
        return;
    }

    if (be32toh(header.index_major) != CTF_INDEX_MAJOR) {
        std::cerr << "Error: incompatible index file" << std::endl;
        fclose(fp);
        return;
    }

    uint32_t packetIndexLen = be32toh(header.packet_index_len);
    if (packetIndexLen == 0) {
        std::cerr << "Error: packet index lenght cannot be 0" << std::endl;
        fclose(fp);
        return;
    }

    CtfPacketIndex *ctfIndex = (CtfPacketIndex*) calloc(packetIndexLen, sizeof(char));
    const auto &traceInfos = *trace.getTracesInfos().begin();
    const auto &clockInfos = traceInfos->getClockInfos();

    while (fread(ctfIndex, packetIndexLen, 1, fp) == 1) {
        PacketHeader index;
        memset(&index, 0, sizeof(index));
        index.offset = be64toh(ctfIndex->offset);
        index.packetSize = be64toh(ctfIndex->packetSize);
        index.contentSize = be64toh(ctfIndex->contentSize);
        index.tsCycles.timestampBegin = be64toh(ctfIndex->timestampBegin);
        index.tsCycles.timestampEnd = be64toh(ctfIndex->timestampEnd);
        index.tsReal.timestampBegin = ctf_get_real_timestamp(clockInfos, index.tsCycles.timestampBegin);
        index.tsReal.timestampEnd = ctf_get_real_timestamp(clockInfos, index.tsCycles.timestampEnd);
        index.eventsDiscarded = be64toh(ctfIndex->eventsDiscarded);
        index.eventsDiscardedLen = 64;
        index.dataOffset = -1;
        this->streamId = be64toh(ctfIndex->streamId);
        indices.emplace_back(std::move(index));
    }

    free(ctfIndex);
    fclose(fp);
}

const std::vector<PacketHeader> &PacketIndex::getPacketIndex() const
{
    return indices;
}

int PacketIndex::getStreamId() const
{
    return streamId;
}
