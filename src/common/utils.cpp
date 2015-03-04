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

#include "utils.h"

#include <cmath>
#include <sstream>

tibee::trace::event_id_t getEventId(const tibee::trace::TraceSet &set, const std::string eventName)
{
    auto &tracesInfos = set.getTracesInfos();
    for (const auto &traceInfos : tracesInfos) {
        if (traceInfos->getTraceType() == "lttng-kernel") {
            for (const auto &eventNameIdPair : *traceInfos->getEventMap()) {
                if (eventNameIdPair.first == eventName) {
                    return eventNameIdPair.second->getId();
                }
            }
        }
    }
    return -1;
}

double logbase(double x, double base) {
    return log(x)/log(base);
}

std::string convertSize(uint64_t size) {
    if (size <= 0) {
        return "0 B";
    }
    std::string sizeName[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = static_cast<int>(floor(logbase(size, 1024)));
    double p = pow(1024, i);
    double s = roundf((size/p) * 100) / 100;
    if (s > 0) {
        std::stringstream ss;
        ss << s << " " << sizeName[i];
        return ss.str();
    } else {
        return "0 B";
    }
}
