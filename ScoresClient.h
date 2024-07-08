#ifndef SCORESCLIENT_H
#define SCORESCLIENT_H

#include <QTcpSocket>
#include <QDataStream>
#include <QDateTime>
#include <QAbstractListModel>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QTextStream>
#include <QStringList>
#include <QUrl>

class DataModel : public QAbstractListModel {
    Q_OBJECT
    using QAbstractListModel::QAbstractListModel;

public:
    struct Data {
        int index;
        QTime bestLapTime;
        QTime averageLapTime;
    };

    enum DataRoles {
        IndexRole = Qt::UserRole + 1,
        BestLapTimeRole,
        AverageLapTimeRole,
        BestLapDisplayRole
    };

    void setScores(const QList<Data> &dataList);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    QList<Data> m_data;
};

class SortedModel : public QSortFilterProxyModel {
    Q_OBJECT
    using QSortFilterProxyModel::QSortFilterProxyModel;

public:
    enum DataRoles {
        Difference = DataModel::BestLapDisplayRole + 1,
    };

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;
};

class ScoresClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(SortedModel* scores READ scores CONSTANT)

public:
    ScoresClient(QString host, int port, QObject *parent = nullptr);

    DataModel* model() const {
        return m_model;
    }

    SortedModel* scores() const {
        return m_scores;
    }

    Q_INVOKABLE void sendMeasurement(int index, qint64 timestamp);

    Q_INVOKABLE void getScores(QUrl filename);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket;
    QString m_host;
    int m_port;

    DataModel* m_model;
    SortedModel* m_scores;
};

#endif // SCORESCLIENT_H
