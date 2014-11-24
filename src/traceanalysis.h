#ifndef TRACEANALYSIS_H
#define TRACEANALYSIS_H

#include <QObject>

class TraceAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit TraceAnalysis(QObject *parent = 0);

    int getThreads() const;
    void setThreads(int value);

    QString getTracePath() const;
    void setTracePath(const QString &value);

    bool getVerbose() const;
    void setVerbose(bool value);

signals:
    void finished();

public slots:
    void execute();

protected:
    virtual void doExecute() = 0;

private:
    int threads;
    QString tracePath;
    bool verbose;
};

#endif // TRACEANALYSIS_H
