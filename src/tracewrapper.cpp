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
