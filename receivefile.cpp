#include "receivefile.h"

ReceiveFile::ReceiveFile(QObject *parent)
    : QObject{parent}
{
    this->tcpserver=new QTcpServer();
    connect(tcpserver,&QTcpServer::newConnection,this,&ReceiveFile::TcpHandler);
}


bool ReceiveFile::startlisten(quint16 port){
    if(!tcpserver||port>=65535) return false;
    if(!tcpserver->listen(QHostAddress::AnyIPv4,port)){
        //qDebug()<<tcpserver->serverError();
        emit exceptionThrow(ErrorInfo::PORT_BUSY,"当前监听端口"+QString::number(port)+"繁忙\n"+qint16(tcpserver->serverError())+tcpserver->errorString());
        return false;
    };
    return true;
}

QString ReceiveFile::computeHash(const QString& path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) return "";

    QCryptographicHash hash(QCryptographicHash::Md5);
    qint64 filesize=file.size();
    //constexpr qint64 MaxBuffer = 64*1024;
    qint64 Bytes=qMin(filesize,MaxBuffer);
    char payload[Bytes];
    //状态，status返回read实际读取到的字节，末尾=0，失败=-1
    int status;
    while(filesize>=0&&(status=file.read(payload,Bytes))>0){
        filesize -= status;
        hash.addData(payload, status);
        Bytes=qMin(filesize,MaxBuffer);
    }
    file.close();
    this->filehash=hash.result().toHex();
    return filehash;
}

void ReceiveFile::TcpHandler(){
    qDebug()<<"接收到连接";

    socket=tcpserver->nextPendingConnection();
    //绑定断开检测
    connect(socket,&QTcpSocket::disconnected,this,[this](){
        QTcpSocket *send=(QTcpSocket *)sender();
        qDebug()<<QString::number(float(receivedBytes))<<QString::number(float(filesize));
        if(receivedBytes==filesize) {
            acceptedheader=false;
            qDebug()<<"Serverdisconnected";
        }

        if(file->isOpen()) file->close();
        emit receivedSucc(computeHash(filepath+"/"+filename)==filehash);

    });
    //绑定接收数据并写入信号
    connect(socket,&QTcpSocket::readyRead,this,&ReceiveFile::ReadyReadHandler, Qt::UniqueConnection);

    emit newConn(socket->peerAddress(),socket->peerPort());

    return;
}

void ReceiveFile::ReadyReadHandler(){
    QTcpSocket *send=(QTcpSocket *)sender();
    data.append(send->readAll());

    qDebug()<<"开始读取";
    if(!acceptedheader){
        if (data.size() < 8 + 8 + 4) return;
        QDataStream in(data); in.setVersion(QDataStream::Qt_6_9);
        char magic[8]; in.readRawData(magic, 8);
        quint32 filenamesize;
        in>> filesize >> filenamesize;
        if (data.size() < 8 + 8 + 4 + int(filenamesize) + 2) return;
        filename.resize(filenamesize); in.readRawData(filename.data(),filenamesize);
        quint16 filehashsize;
        in >> filehashsize;
        if (data.size() < 8 + 8 + 4 + int(filenamesize) + 2 + int(filehashsize)) return;
        filehash.resize(filehashsize); in.readRawData(filehash.data(),filehashsize);
        qDebug()<< "Header:" << QString::fromUtf8(magic)
                 << " size:"<< filesize
                 << " name:"<<filename
                 <<" hash:"<<filehash;
        qDebug()<<"length:"<<8+8+4+filename.size()+2+filehash.size();
        acceptedheader=true;
        const quint64 removelen =
            sizeof(magic)         // 8 magic
            +   sizeof(filesize)      // 8 fileSize
            +   sizeof(filenamesize)  // 4 nameLen
            +   filename.size()  //name bytes
            +   sizeof(filehashsize)  // 2 hashLen
            +   filehash.size(); //hash bytes

        qDebug()<<"准备移除字节"<<removelen;
        data.remove(0,removelen);
        receivedBytes=0;
        file=new QFile(filepath+"/"+filename);
        if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qDebug() << "文件打开失败：" << file->errorString();
            return;
        }
    }
    if(acceptedheader){
        //QFile file(filepath+"/"+filename);
        while (!data.isEmpty() && receivedBytes < filesize) {
            qint64 len=qMin<qint64>(data.size(),qint64(filesize-receivedBytes));
            qDebug()<<"接收帧大小:"<<len;
            qint64 out=file->write(data.constData(),len);
            if(out<=0) break;
            qDebug()<<"写入文件大小:"<<QString::number(out);
            data.remove(0,out);
            receivedBytes+=out;
        }

        if (receivedBytes == filesize) {
            emit respACK();
            if (file && file->isOpen()) { /*file->flush();*/ file->close(); }
            QJsonObject ok{{"type","ACK_FILE"},{"ok",true},{"size",qint64(filesize)},{"filehash",filehash.data()}};
            qDebug()<<"回复成功ACK"<<ok;
            QByteArray pl = QJsonDocument(ok).toJson(QJsonDocument::Compact);
            QByteArray frame; QDataStream out(&frame, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_6_9); out << quint32(pl.size()); frame.append(pl);
            socket->write(frame);
        }
    }
}

void ReceiveFile::closeall(){
    if(tcpserver->isListening()) tcpserver->close();
    //if(socket->)
}

