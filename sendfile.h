#ifndef SENDFILE_H
#define SENDFILE_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QDataStream>
#include <QTimer>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>

#define MaxQueuedSize (1<<20)

class SendFile : public QObject
{
    Q_OBJECT
public:
    explicit SendFile(QObject *parent = nullptr);
    explicit SendFile(QHostAddress target,quint16 port,QString file,QObject *parent = nullptr);

    QHostAddress targetip;
    quint16 targetport;
    QString filepath;
    QFile *fileobj;
    QByteArray filehash;
    QByteArray fileheader;
    QTcpSocket *tcpsocket;
    bool isConnected;
    quint64 sentsize{};
    void TCPconnect();
    void startsend();

private:

    quint64 maxqueued{MaxQueuedSize};
    qint64 MaxBuffer = 64*1024;
    bool awaitingAck{false};
    QByteArray ackData;
    QByteArray parseheader(const QString& path);
    QString computeHash(const QString& path);
    QString computeHash(QByteArray data);
    void PumpData();

private slots:
    void onConnected();
    void onBytesWritten(qint64 writtenByte);
    void onDisconnected();

    //void onError();
signals:
    void progressnum(quint16 num);
    void ACKreceiver(bool ACKreceiver,const QString& verifyhash);


};

#endif // SENDFILE_H
