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

#include <endian.h>

#define CTF_INDEX_MAGIC 0xC1F1DCC1
#define CTF_INDEX_MAJOR 1
#define CTF_INDEX_MINOR 0

PacketIndex::PacketIndex(std::string packetIndexPath)
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

    uint32_t packet_index_len = be32toh(header.packet_index_len);
    if (packet_index_len == 0) {
        std::cerr << "Error: packet index lenght cannot be 0" << std::endl;
        fclose(fp);
        return;
    }

    ctf_packet_index *ctf_index = (ctf_packet_index*) calloc(packet_index_len, sizeof(char));
    while (fread(ctf_index, packet_index_len, 1, fp) == 1) {
        packet_index index;
        memset(&index, 0, sizeof(index));
        index.offset = be64toh(ctf_index->offset);
        index.packet_size = be64toh(ctf_index->packet_size);
        index.content_size = be64toh(ctf_index->content_size);
        index.ts_cycles.timestamp_begin = be64toh(ctf_index->timestamp_begin);
        index.ts_cycles.timestamp_end = be64toh(ctf_index->timestamp_end);
        index.events_discarded = be64toh(ctf_index->events_discarded);
        index.events_discarded_len = 64;
        index.data_offset = -1;
        indices.emplace_back(std::move(index));
    }

    free(ctf_index);
    fclose(fp);
}

const std::vector<packet_index> &PacketIndex::getPacketIndex() const
{
    return indices;
}
