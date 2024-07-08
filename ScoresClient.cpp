#include "ScoresClient.h"
#include "CommTypes.h"
#include <QFile>

void DataModel::setScores(const QList<Data> &dataList) {
    beginResetModel();
    m_data = dataList;
    endResetModel();
}

int DataModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_data.size();
}

QVariant DataModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    const Data &item = m_data.at(index.row());
    if (role == IndexRole) {
        return item.index;
    } else if (role == BestLapTimeRole) {
        return item.bestLapTime;
    } else if (role == AverageLapTimeRole) {
        return item.averageLapTime.toString("mm:ss.zzz");
    } else if (role == BestLapDisplayRole) {
        return item.bestLapTime.toString("mm:ss.zzz");
    }

    return QVariant();
}

QHash<int, QByteArray> DataModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "index";
    roles[BestLapTimeRole] = "time";
    roles[AverageLapTimeRole] = "averagelap";
    roles[BestLapDisplayRole] = "bestlap";
    return roles;
}

QVariant SortedModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == SortedModel::Difference)
    {
        QTime t = index.data(DataModel::BestLapTimeRole).toTime();

        if (index.row() > 0)
        {
            auto first = index.siblingAtRow(0);
            QTime bestLap = first.data(DataModel::BestLapTimeRole).toTime();

            QString d = QString("+%1 s").arg(QString::number(double(bestLap.msecsTo(t))/1000.0, 'f', 1));
            return QVariant(d);
        }
        else
        {
            // return empty string for leader (no gap)
            return QVariant(QString());
        }
    }
    else
    {
        return QSortFilterProxyModel::data(index, role);
    }
}

QHash<int, QByteArray> SortedModel::roleNames() const {
    QHash<int, QByteArray> roles = QSortFilterProxyModel::roleNames();
    roles[SortedModel::Difference] = "diff";
    return roles;
}

ScoresClient::ScoresClient(QString host, int port, QObject *parent) :
    QObject(parent),
    m_socket(new QTcpSocket(this)),
    m_host(host),
    m_port(port),
    m_model(new DataModel(this)),
    m_scores(new SortedModel(this))
{
    m_scores->setSourceModel(m_model);
    m_scores->setSortRole(DataModel::BestLapTimeRole);
    m_scores->setDynamicSortFilter(true);

    connect(m_socket, &QTcpSocket::connected, this, &ScoresClient::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &ScoresClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ScoresClient::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, &ScoresClient::onError);
}

void ScoresClient::sendMeasurement(int index, qint64 timestamp) {
    m_socket->connectToHost(m_host, m_port);
    if (m_socket->waitForConnected(3000))
    {
        qDebug() << "Client: Sending measurement";

        QByteArray request;
        QDataStream out(&request, QIODevice::WriteOnly);
        out << ScoresComm::MessageType::SaveMeasurement << index << timestamp;

        m_socket->write(request);
        m_socket->flush();
        qDebug() << "Client: Sending measurement SENT";
    }
    else
    {
        qDebug() << "Client: Sending measurement: Failed to connect to the server.";
    }
}

void ScoresClient::getScores(QUrl filename) {
    m_socket->connectToHost(m_host, m_port);
    if (m_socket->waitForConnected(3000))
    {

        qDebug() << "Client: Reading racing data from file:" << filename.path();

        QFile file(filename.path());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Client: Cannot open file for reading:" << file.errorString();
            return;
        }

        QTextStream in(&file);
        QList<QPair<int, qint64>> data;
        while (!in.atEnd())
        {
            QString line = in.readLine();
            QStringList fields = line.split(',');

            // data is kart number, timeOfDay
            if (fields.length() == 2)
            {
                bool ok = false;
                int kartId = fields.at(0).toInt(&ok);
                QDateTime t = QDateTime::fromString(fields.at(1),"hh:mm:ss");

                if (ok && t.isValid())
                {
                    data.append(qMakePair(kartId, t.toMSecsSinceEpoch()));
                }
            }
            else
            {
                qWarning() << "Invalid racing data in the file" << fields;
            }
        }

        file.close();

        qDebug() << "Client: Requesting scores";

        QByteArray request;
        QDataStream out(&request, QIODevice::WriteOnly);
        out << ScoresComm::MessageType::GetScores << data;

        m_socket->write(request);
        m_socket->flush();
    }
    else
    {
        qDebug() << "Client: Request scores: Failed to connect to the server.";
    }
}

void ScoresClient::onConnected() {
    qDebug() << "Client: Connected to server";
}

void ScoresClient::onReadyRead() {
    QDataStream in(m_socket);
    if (m_socket->bytesAvailable() < (qint64)sizeof(QList<QPair<int, qint64>>))
    {
        qDebug() << "Client: received invalid data.";
        return;
    }
    qDebug() << "Client: received data.";

    QList<QVariantList> dataList;
    in >> dataList;

    QList<DataModel::Data> modelData;
    for (const auto &list : dataList)
    {
        modelData.append({
            list.at(0).toInt(),
            QTime::fromMSecsSinceStartOfDay(list.at(1).toLongLong()),
            QTime::fromMSecsSinceStartOfDay(list.at(2).toLongLong())
        });
    }

    m_model->setScores(modelData);
    m_scores->sort(0, Qt::AscendingOrder);
}

void ScoresClient::onDisconnected() {
    qDebug() << "Disconnected from server";
}

void ScoresClient::onError(QAbstractSocket::SocketError socketError) {
    qWarning() << "Socket error:" << socketError << m_socket->errorString();
}
