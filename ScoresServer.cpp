#include "ScoresServer.h"
#include "CommTypes.h"

ScoresServer::ScoresServer(int port, QObject *parent) : QTcpServer(parent)
{
    if (!this->listen(QHostAddress::Any, port))
    {
        qFatal("Could not start server");
    }
    else
    {
        qDebug() << "Listening on port" << port;
    }

    connect(this, &QTcpServer::newConnection, this, &ScoresServer::handleMessage);
}

void ScoresServer::handleMessage()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_5);

    QTcpSocket *socket = this->nextPendingConnection();
    connect(socket, &QAbstractSocket::disconnected,
            socket, &QObject::deleteLater);

    connect(socket, &QTcpSocket::readyRead, this, [=]() {
        QDataStream in(socket);
        if (socket->bytesAvailable() < (qint64)sizeof(int))
        {
            qDebug() << "Server received invalid data.";
            return;
        }

        ScoresComm::MessageType requestType;
        in >> requestType;

        if (requestType == ScoresComm::MessageType::SaveMeasurement)
        {
            if (socket->bytesAvailable() < (qint64)(sizeof(int) + sizeof(qint64)))
            {
                qDebug() << "Server received invalid data.";
                return;
            }

            int kartId;
            qint64 timestamp;
            in >> kartId >> timestamp;

            registerLap(kartId, timestamp);
        }
        else if (requestType == ScoresComm::MessageType::GetScores)
        {
            if (socket->bytesAvailable() < (qint64)sizeof(QList<QPair<int, qint64>>))
            {
                qDebug() << "Server received invalid data.";
                return;
            }
            qDebug() << "Client: received data.";

            QList<QPair<int, qint64>> dataList;
            in >> dataList;

            for (const auto &pair : dataList)
            {
                registerLap(pair.first, pair.second);
            }
        }
        else
        {
            qWarning() << "Server: Received unknown message!";
        }

        QList<QVariantList> data;
        QMapIterator<int, RaceDataItem> i(m_raceData);
        while (i.hasNext()) {
            i.next();
            if (i.value().bestLapTime > 0) {
                data.append(QVariantList({i.key(), i.value().bestLapTime, i.value().averageLap}));
            }
        }

        QByteArray response;
        QDataStream out(&response, QIODevice::WriteOnly);
        out << data;

        socket->write(response);
        socket->flush();
        qDebug() << "Server: Scores sent";

        socket->disconnectFromHost();
    });
}

void ScoresServer::registerLap(int kartId, qint64 timestamp)
{
    QDateTime measurement = QDateTime::fromMSecsSinceEpoch(timestamp);

    if (m_raceData.contains(kartId))
    {
        qint64 currentLap = m_raceData[kartId].time.msecsTo(measurement);
        qint64 currentBest = m_raceData[kartId].bestLapTime;
        m_raceData[kartId].totalTime += currentLap;
        m_raceData[kartId].currentLap++;
        m_raceData[kartId].averageLap = m_raceData[kartId].totalTime / m_raceData[kartId].currentLap;

        if (currentBest < 0 || currentLap < currentBest)
        {
            // Save current as best lap
            m_raceData[kartId].bestLapTime = currentLap;
        }
    }

    // Save current time
    m_raceData[kartId].time = measurement;

    qDebug() << "Registered measurement - kartId:" << kartId << "timestamp:" << measurement;
}
