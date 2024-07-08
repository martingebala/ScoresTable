#ifndef SCORESSERVER_H
#define SCORESSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QDateTime>
#include <QList>
#include <QDebug>

class ScoresServer : public QTcpServer {
    Q_OBJECT

public:
    ScoresServer(int port, QObject *parent = nullptr);

    void handleMessage();

private:
    struct RaceDataItem {
        QDateTime time;
        qint64 bestLapTime {-1};
        qint64 totalTime {0};
        qint64 currentLap {0};
        qint64 averageLap {0};
    };
    QMap<int, RaceDataItem> m_raceData;

    void registerLap(int kartId, qint64 timestamp);
};

#endif // SCORESSERVER_H
