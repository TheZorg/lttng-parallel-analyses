#include "traceanalysis.h"
#include "countanalysis.h"

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
    QString analysis;
    int threads;
    bool verbose;
    QString tracePath;
};

QStringList analysisList = QStringList() << "count" << "cpu" << "io";

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, Options &opts, QString *errorMessage) {
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    // Verbose
    const QCommandLineOption verboseOption(QStringList() << "V" << "verbose", "Be verbose.");
    parser.addOption(verboseOption);

    // Number of threads to use
    const QCommandLineOption threadOption(QStringList() << "t" << "thread", "Maximum number of threads to use.",
                                          "num threads", "4");
    parser.addOption(threadOption);

    // Analysis name
    const QCommandLineOption analysisOption(QStringList() << "a" << "analysis", "Type of analysis to execute [ count | cpu | io ].",
                                            "analysis name", "count");
    parser.addOption(analysisOption);

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

    const QString threadsString = parser.value(threadOption);
    int threads = threadsString.toInt();
    if (threads <= 0) {
        *errorMessage = "Number of threads must be 1 or more.";
        return CommandLineParseResult::Error;
    }
    opts.threads = threads;

    const QString analysisString = parser.value(analysisOption);
    if (!analysisList.contains(analysisString)) {
        *errorMessage = "Invalid analysis.";
        return CommandLineParseResult::Error;
    }
    opts.analysis = analysisString;

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

TraceAnalysis* getAnalysisFromName(QString analysisName, QCoreApplication *app) {
    if (analysisName == "count") {
        return new CountAnalysis(app);
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
        std::cout << "--analysis : " << qPrintable(opts.analysis) << std::endl;
        std::cout << "<path/to/trace> : " << qPrintable(opts.tracePath) << std::endl;
    }

    TraceAnalysis *analysis = getAnalysisFromName(opts.analysis, &a);
    if (analysis == nullptr) {
        std::cerr << "This analysis has not yet been implemented.";
        std::cerr << std::endl << std::endl;
        std::cerr << qPrintable(parser.helpText());
        return 1;
    }

    analysis->setThreads(opts.threads);
    analysis->setTracePath(opts.tracePath);
    analysis->setVerbose(opts.verbose);

    QObject::connect(analysis, SIGNAL(finished()), &a, SLOT(quit()));

    QTimer::singleShot(0, analysis, SLOT(execute()));

    return a.exec();
}
