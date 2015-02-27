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
#include <sys/types.h>
#include <vector>

/*
 * Header at the beginning of each index file.
 * All integer fields are stored in big endian.
 */
struct ctf_packet_index_file_hdr {
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
struct ctf_packet_index {
    uint64_t offset;		/* offset of the packet in the file, in bytes */
    uint64_t packet_size;		/* packet size, in bits */
    uint64_t content_size;		/* content size, in bits */
    uint64_t timestamp_begin;
    uint64_t timestamp_end;
    uint64_t events_discarded;
    uint64_t stream_id;
} __attribute__((__packed__));

struct packet_index_time {
    uint64_t timestamp_begin;
    uint64_t timestamp_end;
};

struct packet_index {
    off_t offset;		/* offset of the packet in the file, in bytes */
    int64_t data_offset;	/* offset of data within the packet, in bits */
    uint64_t packet_size;	/* packet size, in bits */
    uint64_t content_size;	/* content size, in bits */
    uint64_t events_discarded;
    uint64_t events_discarded_len;	/* length of the field, in bits */
    struct packet_index_time ts_cycles;	/* timestamp in cycles */
    struct packet_index_time ts_real;	/* realtime timestamp */
};

class PacketIndex
{
private:
    std::string path;
    ctf_packet_index_file_hdr header;
    std::vector<packet_index> indices;
public:
    PacketIndex(std::string packetIndexPath);
    const std::vector<packet_index> &getPacketIndex() const;
};

#endif // PACKETINDEX_H
