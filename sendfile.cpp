#include "sendfile.h"

SendFile::SendFile(QObject *parent)
    : QObject{parent}
{
}
SendFile::SendFile(QHostAddress target,quint16 port,QString file,QObject *parent)
    :targetip(target),targetport(port),filepath(file),QObject(parent)
{
    this->tcpsocket=new QTcpSocket();
    fileobj=new QFile(filepath);
    connect(tcpsocket, &QTcpSocket::connected,    this, &SendFile::onConnected);
    connect(tcpsocket, &QTcpSocket::bytesWritten, this, &SendFile::onBytesWritten);
    //我在ui绑定，这先注释掉了
    //connect(tcpsocket, &QTcpSocket::disconnected, this, &SendFile::onDisconnected);
    // 监听 ACK
    connect(tcpsocket, &QTcpSocket::readyRead, this, [this]{
        ackData.append(tcpsocket->readAll());
        while (true) {
            if (ackData.size() < 4) break;
            QDataStream in(ackData); in.setVersion(QDataStream::Qt_6_9);
            quint32 len; in >> len;
            if (ackData.size() < 4 + int(len)) break;
            const QByteArray js = ackData.mid(4, len);
            ackData.remove(0, 4 + int(len));
            const auto o = QJsonDocument::fromJson(js).object();
            if (o.value("type")=="ACK_FILE" && o.value("ok").toBool()) {
                qDebug()<<"收到ACK_disconnected 对比hash"<<o.value("filehash").toString()<< filehash;
                awaitingAck = false;
                emit ACKreceiver(true,o.value("filehash").toString());
                tcpsocket->disconnectFromHost();
            }
        }
    });
    fileheader=parseheader(filepath);

}

SendFile::~SendFile() {
    // ✅ 确保TCP连接正确关闭
    if (tcpsocket) {
        if (tcpsocket->state() != QAbstractSocket::UnconnectedState) {
            tcpsocket->disconnectFromHost();
            if (!tcpsocket->waitForDisconnected(3000)) {
                tcpsocket->abort();
            }
        }
        tcpsocket->deleteLater();
    }
    
    // ✅ 确保文件正确关闭
    if (fileobj) {
        if (fileobj->isOpen()) {
            fileobj->close();
        }
        delete fileobj;
    }
    
    qDebug() << "SendFile destroyed, resources cleaned up";
}

QByteArray SendFile::parseheader(const QString& path){
    QByteArray filename=QFileInfo(*fileobj).fileName().toUtf8();

    QByteArray header;
    QBuffer headerbuffer(&header); headerbuffer.open(QIODevice::ReadWrite);
    QDataStream stream(&headerbuffer); stream.setVersion(QDataStream::Qt_6_9);
    stream.writeRawData("tranfile",8);
    computeHash(filepath);

    //流创建header:魔数+文件大小+文件名
    stream << quint64(fileobj->size());
    stream << quint32(filename.size());
    //headerbuffer.write(filename);
    stream.writeRawData(filename,filename.size());
    stream << quint16(filehash.size());
    //headerbuffer.write(filehash);
    stream.writeRawData(filehash,filehash.size());
    //stream << quint32(header.size());
    // QDataStream in(header); in.setVersion(QDataStream::Qt_6_9);
    // char magic[8]; in.readRawData(magic, 8);
    // char data[100],data1[100];
    // quint64 a;quint32 b;qint16 c;QString n,h;
    // in>>a;
    // qDebug() << a;
    // in >> b;
    // in.readRawData(data,b);
    // qDebug() << data;

    // in >> c;
    // in.readRawData(data1,c);
    // qDebug() << data1;
     qDebug() <<"header:"<< header;
     qDebug() <<"headerdata"<<header.data();
    // //qDebug()<< "a:"<<a <<" n:"<<n<<" h:"<<h;
    return header;
}

QString SendFile::computeHash(const QString& path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return "";

    QCryptographicHash hash(QCryptographicHash::Md5);
    qint64 filesize=file.size();
    qint64 Bytes=qMin(filesize,MaxBuffer);
    
    // ✅ 修复：使用QByteArray替代变长数组
    QByteArray payload(Bytes, 0);
    
    int status;
    while(filesize>=0&&(status=file.read(payload.data(),Bytes))>0){
        filesize -= status;
        hash.addData(payload.constData(), status);
        Bytes=qMin(filesize,MaxBuffer);
        
        // ✅ 添加：动态调整缓冲区大小
        if (payload.size() != Bytes) {
            payload.resize(Bytes);
        }
    }
    file.close();
    this->filehash=hash.result().toHex();
    return filehash;
}

void SendFile::TCPconnect(){
    // ✅ 修复：检查连接状态，避免重复连接
    if (tcpsocket->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << "Socket already connected or connecting, state:" << tcpsocket->state();
        return;
    }
    
    qDebug() << "Connecting to" << targetip << ":" << targetport;
    // ✅ 修复：使用构造函数传入的端口而不是硬编码
    tcpsocket->connectToHost(targetip, targetport);
}

void SendFile::onConnected(){
    this->isConnected=true;
    if(!fileobj->open(QIODevice::ReadOnly)) return;
    if (!fileobj->isOpen() && !fileobj->open(QIODevice::ReadOnly)) {
        tcpsocket->disconnectFromHost();
        return;
    }
    sentsize=0;
    tcpsocket->write(fileheader);
    qDebug()<<"发送header总长度："<<fileheader.size();
    PumpData();
}
void SendFile::startsend(){
    // ✅ 修复：统一使用TCPconnect，避免代码重复
    TCPconnect();
}
void SendFile::onDisconnected(){
    //emit progressnum(qint16(100*float(sentsize)/float(fileobj->size())));
}
void SendFile::onBytesWritten(qint64 writtenByte){
    // ✅ 修复：正确跟踪已发送的字节数
    qDebug()<<"已写入数据"<<writtenByte;

    if(fileobj->atEnd() && tcpsocket->bytesToWrite() == 0&&qint64(sentsize)==fileobj->size()) {
        awaitingAck = true;
        QTimer::singleShot(3000, this, [this]{
            if (awaitingAck) {
                qDebug()<<"超时disconnected";
                emit ACKreceiver(false,"");
                tcpsocket->disconnectFromHost();
                awaitingAck=false;
            }
        });
    }
    PumpData();
    qDebug()<<QString::number(sentsize)<<QString::number(fileobj->size());
    emit progressnum(qint16(round(100*float(sentsize)/float(fileobj->size()))));
    return;
}
void SendFile::PumpData(){
    qDebug()<<"start send";
    //while(tcpsocket->bytesToWrite() < maxqueued&&!fileobj->atEnd()){
    //    quint64 len=qMin(quint64(MaxBuffer),quint64(fileobj->size())-sentsize);
    while(tcpsocket->bytesToWrite() < maxqueued && !fileobj->atEnd()){
        // ✅ 修复：基于文件剩余大小计算读取长度
        qint64 remainingBytes = fileobj->size() - fileobj->pos();
        quint64 len = qMin(quint64(MaxBuffer), quint64(remainingBytes));

        QByteArray data=fileobj->read(qint64(len));
        qDebug()<<"发送帧大小:"<<len;
        qint64 out=tcpsocket->write(data);
        //qDebug()<<QString::number(float(sentsize)/float(fileobj->size()));
        //emit progressnum(qint16(100*float(sentsize)/float(fileobj->size())));
        if(data.isEmpty()||out<=0) break;
        sentsize+=out;
    }
    qDebug()<<"end send";

    return;
}
