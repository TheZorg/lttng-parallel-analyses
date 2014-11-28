#include "countanalysis.h"

#include <QList>
#include <QString>
#include <QtConcurrent/QtConcurrent>

#include <iostream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#include <trace/TraceSet.hpp>
#include <base/BasicTypes.hpp>

using namespace tibee;
using namespace tibee::trace;

static void doSum(int &finalResult, const int &intermediate);
static int doCount(CountWorker &worker);

void CountAnalysis::doExecuteParallel() {
    timestamp_t positions[threads];
    std::vector<CountWorker> workers;
    timestamp_t begin, end;

    {
        // Open a trace to get the begin/end timestamps
        TraceSet set;
        set.addTrace(this->tracePath.toStdString());

        // Get begin timestamp
        begin = set.getBegin();

        // Get end timestamp
        end = set.getEnd();

        // We don't need that context anymore, dispose of it
        // Destructor
    }

    // Calculate begin/end timestamp pairs for each chunk
    timestamp_t step = (end - begin)/threads;

    for (int i = 0; i < threads; i++)
    {
        positions[i] = begin + (i*step);
    }

    // Build the params list
    for (int i = 0; i < threads; i++)
    {
        timestamp_t *begin, *end;
        if (i == 0) {
            begin = nullptr;
        } else {
            begin = &positions[i];
        }
        if (i == threads - 1) {
            end = nullptr;
        } else {
            end = &positions[i+1];
        }
        workers.emplace_back(i, tracePath, begin, end, verbose);
    }

    // Launch map reduce
    QFuture<int> countFuture = QtConcurrent::mappedReduced(workers.begin(), workers.end(), doCount, doSum);

    int count = countFuture.result();

    std::string line(80, '-');

    // Format the count with thousand seperators
    std::stringstream countss;
    countss.imbue(std::locale(""));
    countss << std::fixed << count;

    std::cout << line << std::endl;
    std::cout << "Result of count analysis" << std::endl << std::endl;
    std::cout << std::setw(20) << std::left << "Number of events" << countss.str() << std::endl;
    std::cout << line << std::endl;
}

int doCount(CountWorker &worker) {
    TraceSet &traceSet = worker.getTraceSet();
    TraceSet::Iterator iter = traceSet.between(worker.getBeginPos(), worker.getEndPos());
    TraceSet::Iterator endIter = traceSet.end();

    int count = 0;
    for ((void)iter; iter != endIter; ++iter) {
        count++;
    }

    if (worker.getVerbose()) {
        const timestamp_t *begin = worker.getBeginPos();
        const timestamp_t *end = worker.getEndPos();
        std::string beginString = begin ? std::to_string(*begin) : "START";
        std::string endString = end ? std::to_string(*end) : "END";
        std::cout << "Worker " << worker.getId() << " counted " << count << " events between timestamps "
                  << beginString << " and " << endString << std::endl;
    }

    return count;
}

void CountAnalysis::doExecuteSerial() {
    std::cerr << "Not implemented." << std::endl;
}

void doSum(int &finalResult, const int &intermediate)
{
    finalResult += intermediate;
}

CountWorker::CountWorker(int id, QString path, timestamp_t *begin, timestamp_t *end, bool verbose) :
    TraceWorker(id, path, begin, end, verbose)
{
}

CountWorker::CountWorker(CountWorker &&other) :
    TraceWorker(std::move(other))
{
}
