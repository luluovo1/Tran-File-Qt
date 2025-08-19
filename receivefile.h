#ifndef RECEIVEFILE_H
#define RECEIVEFILE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QBuffer>
#include <QFile>
#include <QDataStream>
#include <QHostAddress>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>

class ReceiveFile : public QObject
{
    Q_OBJECT
public:
    explicit ReceiveFile(QObject *parent = nullptr);

    enum class ErrorInfo{
        PORT_BUSY,
        TCP_CONNECT_FAILD,
        RECEIVE_STOP,
        PATH_NO_PERMISSION
    };
    QTcpServer *tcpserver;
    QTcpSocket *socket;
    QByteArray fileheader;
    QByteArray filename;
    QByteArray filehash;
    bool acceptedheader{false};
    qint64 receivedBytes,filesize;
    QString filepath;
    QFile *file;

    bool startlisten(quint16 port);
    void closeall();

private:
    QByteArray data;

    quint64 maxqueued{1<<20};
    qint64 MaxBuffer = 64*1024;

    QString computeHash(const QString& path);
signals:
    void newConn(const QHostAddress& sourceIP,const quint16 sourcePort);
    void receivedSucc(bool EqualhashOrnot);
    void respACK();
    void exceptionThrow(const ReceiveFile::ErrorInfo code,const QString& info);
private slots:
    void TcpHandler();
    void ReadyReadHandler();

};

#endif // RECEIVEFILE_H
