#include "tracewrapper.h"

#include <babeltrace/babeltrace.h>

TraceWrapper::~TraceWrapper() {
    if (ctx) {
        bt_context_put(ctx);
    }
}

bt_context *TraceWrapper::getContext() {
    if (!ctx) {
        ctx = bt_context_create();
        int trace_id = bt_context_add_trace(ctx, tracePath.toStdString().c_str(), "ctf", NULL, NULL, NULL);
        if(trace_id < 0) {
            //                qDebug() << "Failed: bt_context_add_trace";
        }
    }
    return ctx;
}
