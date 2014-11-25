#include "countanalysis.h"

#include <QList>
#include <QString>
#include <QtConcurrent/QtConcurrent>

#include <iostream>
#include <iomanip>

extern "C" {
#include <babeltrace/babeltrace.h>
#include <babeltrace/format.h>
#include <babeltrace/context.h>
#include <babeltrace/iterator.h>
#include <babeltrace/ctf/events.h>
#include <babeltrace/ctf/iterator.h>
#include "babeltrace/ctf/types.h"
}

static void doSum(int &finalResult, const int &intermediate);
static int doCount(CountWorker &worker);

void CountAnalysis::doExecute() {
    bt_iter_pos positions[threads+1];
    std::vector<CountWorker> workers;
    struct bt_iter_pos end_pos;

    // Open a trace to get the begin/end timestamps
    struct bt_context *ctx = bt_context_create();
    int trace_id = bt_context_add_trace(ctx, tracePath.toStdString().c_str(), "ctf", NULL, NULL, NULL);
    if(trace_id < 0)
    {
        std::cerr << "Failed: bt_context_add_trace" << std::endl;
        return;
    }

    end_pos.type = BT_SEEK_LAST;

    // Get begin timestamp
    struct bt_ctf_iter* iter = bt_ctf_iter_create(ctx, NULL, NULL);
    struct bt_ctf_event *event = bt_ctf_iter_read_event(iter);
    uint64_t begin = bt_ctf_get_timestamp(event);

    // Get end timestamp
    bt_iter_set_pos(bt_ctf_get_iter(iter), &end_pos);
    event = bt_ctf_iter_read_event(iter);
    uint64_t end = bt_ctf_get_timestamp(event);

    // We don't need that context anymore, dispose of it
    bt_context_put(ctx);

    // Calculate begin/end timestamp pairs for each chunk
    uint64_t step = (end - begin)/threads;

    positions[0].type = BT_SEEK_BEGIN;
    for (int i = 1; i < threads; i++)
    {
        positions[i].type = BT_SEEK_TIME;
        positions[i].u.seek_time = begin + (i*step);
    }
    positions[threads].type = BT_SEEK_LAST;

    // Build the params list
    for (int i = 0; i < threads; i++)
    {
        workers.push_back(CountWorker(i, tracePath, positions[i], positions[i+1], verbose));
    }

    // Launch map reduce
    QFuture<int> countFuture = QtConcurrent::mappedReduced(workers.begin(), workers.end(), doCount, doSum);

    int count = countFuture.result();

    std::string line(80, '-');

    std::cout << line << std::endl;
    std::cout << "Result of count analysis" << std::endl;
    std::cout << std::setw(20) << std::left << "Number of events" << count << std::endl;
    std::cout << line << std::endl;
}

int doCount(CountWorker &worker) {
    TraceWrapper &wrapper = worker.getWrapper();
    bt_context *ctx = wrapper.getContext();

    const bt_iter_pos *begin = &worker.getBeginPos();
    const bt_iter_pos *end = &worker.getEndPos();
    bt_ctf_iter *iter = bt_ctf_iter_create(ctx, begin, end);

    bt_ctf_event *ctf_event;
    int count = 0;
    while((ctf_event = bt_ctf_iter_read_event(iter))) {
        count++;
        bt_iter_next(bt_ctf_get_iter(iter));
    }

    bt_ctf_iter_destroy(iter);
    bt_context_put(ctx);

    if (worker.getVerbose()) {
        std::cout << "Worker " << worker.getId() << " counted " << count << " events between timestamps "
                  << worker.getBeginPos().u.seek_time << " and " << worker.getEndPos().u.seek_time << std::endl;
    }

    return count;
}

void doSum(int &finalResult, const int &intermediate)
{
    finalResult += intermediate;
}
