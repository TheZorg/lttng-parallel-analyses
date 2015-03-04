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

#include "common/traceanalysis.h"
#include "count/countanalysis.h"
#include "cpu/cpuanalysis.h"
#include "io/ioanalysis.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <iostream>

enum class CommandLineParseResult {
    OK,
    Error,
    VersionRequested,
    HelpRequested,
};

struct Options {
    QString analysisName = "";
    QString analysisType = "";
    int threads = 1;
    bool verbose = false;
    bool benchmark = false;
    QString tracePath = "";
};

QStringList analysisList = QStringList() << "count" << "cpu" << "io";
QStringList typeList = QStringList() << "serial" << "parallel";

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, Options &opts, QString *errorMessage) {
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    // Verbose
    const QCommandLineOption verboseOption(QStringList() << "V" << "verbose", "Be verbose.");
    parser.addOption(verboseOption);

    // Add timing benchmarks
    const QCommandLineOption benchmarkOption(QStringList() << "b" << "benchmark", "Output benchmark times.");
    parser.addOption(benchmarkOption);

    // Number of threads to use
    const QCommandLineOption threadOption(QStringList() << "t" << "thread", "Maximum number of threads to use.",
                                          "num threads", "4");
    parser.addOption(threadOption);

    // Analysis name
    const QCommandLineOption analysisOption(QStringList() << "a" << "analysis", "Name of analysis to execute [ count | cpu | io ].",
                                            "analysis name", "count");
    parser.addOption(analysisOption);

    // Analysis type (serial vs parallel)
    const QCommandLineOption typeOption(QStringList() << "T" << "type", "Type of analysis to execute [ serial | parallel ].",
                                            "analysis type", "parallel");
    parser.addOption(typeOption);

    // Trace directory is a positional argument (i.e. no option)
    parser.addPositionalArgument("<path/to/trace>", "Trace path.");

    if (!parser.parse(QCoreApplication::arguments())) {
        *errorMessage = parser.errorText();
        return CommandLineParseResult::Error;
    }

    if (parser.isSet(helpOption)) {
        return CommandLineParseResult::HelpRequested;
    }

    if (parser.isSet(versionOption)) {
        return CommandLineParseResult::VersionRequested;
    }

    if (parser.isSet(verboseOption)) {
        opts.verbose = true;
    }

    if (parser.isSet(benchmarkOption)) {
        opts.benchmark = true;
    }

    const QString threadsString = parser.value(threadOption);
    int threads = threadsString.toInt();
    if (threads <= 0) {
        *errorMessage = "Number of threads must be 1 or more.";
        return CommandLineParseResult::Error;
    }
    opts.threads = threads;

    const QString analysisString = parser.value(analysisOption);
    if (!analysisList.contains(analysisString)) {
        *errorMessage = "Invalid analysis name.";
        return CommandLineParseResult::Error;
    }
    opts.analysisName = analysisString;

    const QString typeString = parser.value(typeOption);
    if (!typeList.contains(typeString)) {
        *errorMessage = "Invalid analysis type.";
        return CommandLineParseResult::Error;
    }
    opts.analysisType = typeString;

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        *errorMessage = "Argument '<path/to/trace>' missing.";
        return CommandLineParseResult::Error;
    }
    if (positionalArguments.size() > 1) {
        *errorMessage = "Several '<path/to/trace>' arguments provided.";
        return CommandLineParseResult::Error;
    }
    opts.tracePath = positionalArguments.first();

    return CommandLineParseResult::OK;
}

AbstractTraceAnalysis* getAnalysisFromName(QString analysisName, QCoreApplication *app) {
    if (analysisName == "count") {
        return new CountAnalysis(app);
    } else if (analysisName == "cpu") {
        return new CpuAnalysis(app);
    } else if (analysisName == "io") {
        return new IoAnalysis(app);
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("lttng-parallel-analysis");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("This program allows for the parallel analysis of trace files");

    Options opts;
    QString errorMessage;
    switch (parseCommandLine(parser, opts, &errorMessage)) {
    case CommandLineParseResult::OK:
        break;
    case CommandLineParseResult::Error:
        std::cerr << qPrintable(errorMessage);
        std::cerr << std::endl << std::endl;
        std::cerr << qPrintable(parser.helpText());
        return 1;
    case CommandLineParseResult::VersionRequested:
        std::cout << qPrintable(QCoreApplication::applicationName())
                  << " "
                  << qPrintable(QCoreApplication::applicationVersion())
                  << std::endl;
        return 0;
    case CommandLineParseResult::HelpRequested:
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (opts.verbose) {
        std::cout << "Opts : " << std::endl;
        std::cout << "--thread : " << opts.threads << std::endl;
        std::cout << "--analysis : " << qPrintable(opts.analysisName) << std::endl;
        std::cout << "<path/to/trace> : " << qPrintable(opts.tracePath) << std::endl;
    }

    AbstractTraceAnalysis *analysis = getAnalysisFromName(opts.analysisName, &a);
    if (analysis == nullptr) {
        std::cerr << "This analysis has not yet been implemented.";
        std::cerr << std::endl << std::endl;
        std::cerr << qPrintable(parser.helpText());
        return 1;
    }

    analysis->setThreads(opts.threads);
    analysis->setTracePath(opts.tracePath);
    analysis->setVerbose(opts.verbose);
    analysis->setDoBenchmark(opts.benchmark);
    if (opts.analysisType == "parallel") {
        analysis->setIsParallel(true);
    } else if (opts.analysisType == "serial") {
        analysis->setIsParallel(false);
    }

    QObject::connect(analysis, SIGNAL(finished()), &a, SLOT(quit()));

    QTimer::singleShot(0, analysis, SLOT(execute()));

    return a.exec();
}
