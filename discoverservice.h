#ifndef DISCOVERSERVICE_H
#define DISCOVERSERVICE_H

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QTimer>
#include <QVector>

#define ScanPort (121)

#define PayLoadHeader "lulutran"

class DiscoverService : public QObject
{
    Q_OBJECT
public:
    QString udpinfo,instanceID;
    std::atomic<bool> thread_runnable{false};

    enum class BroadcastType {
        BROADCAST=4,
        REQUEST=1,
        RESPONE=2,
        HEARTBEAT=3
    };

    struct DevInfo{
        QString name;
        QHostAddress ip;
        quint16 broadport,transferPort;
        quint64 last_respone_ts;
    };

    QVector<DevInfo> AvailableDev;
    void init();
    explicit DiscoverService(QObject *parent = nullptr);
    QList<QHostAddress> ScanHost();
    void LoopBroadcast();
    void RequestUDP();


private:
    QString DevName{};
    quint16 DiscoverPort{ScanPort},tcpPort{233};//扫描端口，传输端口
    QString udpHeader{PayLoadHeader};//载荷的头部，类似网络请求Header
    QTimer *broadcasttimer;

    QUdpSocket *udpsocket;

    void BroadcastDevice(QUdpSocket *udpsocket,const BroadcastType& type,quint16 discoverport,const QHostAddress& SelfIP,const QHostAddress& targetIP = QHostAddress::Null);
    QByteArray AppendPayload(const QString& udpHeader,const BroadcastType& broadcasttype,const QHostAddress& SelfIP);
    QString getFirstLocalIPv4();
    bool HandleUDP(const QByteArray& data,QHostAddress sender,quint16 port);


signals:
    void accepted(const QString& txt);
    void updateDev();
};

#endif // DISCOVERSERVICE_H
