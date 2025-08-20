#include "receivefile.h"

ReceiveFile::ReceiveFile(QObject *parent)
    : QObject{parent}
    , tcpserver(nullptr)
    , socket(nullptr)
    , acceptedheader(false)
    , receivedBytes(0)
    , filesize(0)
    , file(nullptr)
{
    this->tcpserver=new QTcpServer(this); // 🔧 设置parent防止内存泄漏
    connect(tcpserver,&QTcpServer::newConnection,this,&ReceiveFile::TcpHandler);
}

ReceiveFile::~ReceiveFile() {
    // ✅ 清理TCP服务器
    if (tcpserver) {
        if (tcpserver->isListening()) {
            tcpserver->close();
        }
        tcpserver->deleteLater();
    }
    
    // ✅ 清理当前socket连接
    if (socket) {
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->disconnectFromHost();
            socket->waitForDisconnected(1000);
        }
        socket->deleteLater();
    }
    
    // ✅ 确保文件正确关闭和清理
    if (file) {
        if (file->isOpen()) {
            file->close();
        }
        delete file;
        file = nullptr;
    }
    
    qDebug() << "ReceiveFile destroyed, resources cleaned up";
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

void ReceiveFile::TcpHandler(){
    qDebug()<<"接收到连接";

    socket=tcpserver->nextPendingConnection();
    
    // ✅ 添加：空指针检查
    if (!socket) {
        qDebug() << "Failed to get pending connection";
        return;
    }
    
    // ✅ 添加：检查socket状态
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Socket not in connected state:" << socket->state();
        socket->deleteLater();
        socket = nullptr;
        return;
    }
    
    //绑定断开检测
    connect(socket,&QTcpSocket::disconnected,this,[this](){
        QTcpSocket *send=(QTcpSocket *)sender();
        qDebug()<<QString::number(float(receivedBytes))<<QString::number(float(filesize));
        
        // ✅ 改进：更完善的连接断开处理
        bool isCompleteTransfer = (acceptedheader && receivedBytes == filesize);
        
        if(isCompleteTransfer) {
            acceptedheader=false;
            qDebug()<<"文件传输完成，服务器断开连接";
        } else if (acceptedheader) {
            qDebug()<<"警告：文件传输未完成，连接意外断开. 接收:" 
                   << receivedBytes << "期望:" << filesize;
        }

        if(file && file->isOpen()) {
            file->close();
        }
        
        // 只有在接收完整文件后才进行哈希验证
        if (isCompleteTransfer) {
            emit receivedSucc(computeHash(filepath+"/"+filename).toLower()==filehash.toLower());
        } else {
            qDebug()<<"6";
            emit receivedSucc(false); // 传输不完整
        }
    });
    
    // ✅ 添加：socket错误处理
    connect(socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qDebug() << "Socket断开:" << error << socket->errorString();
        if (file && file->isOpen()) file->close();
        if(receivedBytes!=filesize) emit receivedSucc(false);
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
        
        // ✅ 修复：确保旧文件对象被正确清理
        if (file) {
            if (file->isOpen()) {
                file->close();
            }
            delete file;
            file = nullptr;
        }
        
        file=new QFile(filepath+"/"+filename);
        if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qDebug() << "文件打开失败：" << file->errorString();
            // ✅ 修复：失败时清理文件对象
            delete file;
            file = nullptr;
            // ✅ 添加：发送错误信号
            socket->disconnectFromHost();
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
            // ✅ 修复：正确的哈希序列化方式
            QJsonObject ok{{"type","ACK_FILE"},{"ok",true},{"size",qint64(filesize)},{"filehash",QString(filehash)}};
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

