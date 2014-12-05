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

#include "tracewrapper.h"

#include <iostream>

#include <babeltrace/babeltrace.h>

TraceWrapper::TraceWrapper(QString tracePath) : ctx(NULL), tracePath(tracePath)
{
    ctx = bt_context_create();
    int trace_id = bt_context_add_trace(ctx, this->tracePath.toStdString().c_str(), "ctf", NULL, NULL, NULL);
    if(trace_id < 0) {
        std::cerr << "Failed: bt_context_add_trace" << std::endl;
    }
}

TraceWrapper::TraceWrapper(TraceWrapper &&other) :
    ctx(std::move(other.ctx)), tracePath(std::move(other.tracePath))
{
    other.ctx = nullptr;
}

TraceWrapper &TraceWrapper::operator=(TraceWrapper &&other)
{
    if (this != &other) {
        ctx = std::move(other.ctx);
        other.ctx = nullptr;
        tracePath = std::move(other.tracePath);
    }
    return *this;
}

TraceWrapper::~TraceWrapper() {
    if (ctx) {
        bt_context_put(ctx);
    }
}

bt_context *TraceWrapper::getContext() {
    return ctx;
}
