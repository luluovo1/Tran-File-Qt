#include "discoverservice.h"
#include <QDateTime>

DiscoverService::DiscoverService(QObject *parent)
    : QObject{parent}
{
    qDebug()<<"执行构造函数";
    this->DevName=QHostInfo::localHostName();

}

void DiscoverService::init(){

}

void DiscoverService::BroadcastDevice(QUdpSocket *udpsocket,const BroadcastType& type,quint16 discoverport,const QHostAddress& SelfIP,const QHostAddress& targetIP){
    const QByteArray payload = AppendPayload(udpHeader,type,SelfIP);
    qDebug() << "广播内容:" <<payload.toStdString();
    //udpsocket->writeDatagram(payload,QHostAddress::Broadcast,discoverport);
    //逻辑分开，如果广播包的Method非Broadcast要动态处理发送的对象
    if(!targetIP.isNull()) udpsocket->writeDatagram(payload,targetIP,discoverport);
        else udpsocket->writeDatagram(payload,QHostAddress::Broadcast,discoverport);

    for (const auto& iface : QNetworkInterface::allInterfaces()) {
        auto f = iface.flags();
        if (!(f & QNetworkInterface::IsUp) || !(f & QNetworkInterface::IsRunning)) continue;
        if (f & QNetworkInterface::IsLoopBack) continue;
        for (const auto& e : iface.addressEntries()) {
            if (e.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            if (!e.broadcast().isNull()){
                //qDebug()<< "广播器："<<e.broadcast().toString();
                udpsocket->writeDatagram(payload, e.broadcast(), DiscoverPort);
            }

        }
    }

}

QByteArray DiscoverService::AppendPayload(const QString& udpHeader,const BroadcastType& broadcasttype,const QHostAddress& SelfIP){
    QJsonObject payload{
        {"Header",udpHeader},
        {"Method",static_cast<quint16>(broadcasttype)},
        {"Name",DevName},
        {"Ip",SelfIP.toString()},
        {"Port",tcpPort},
        {"Instance",instanceID},
        {"Ts",qint64(QDateTime::currentSecsSinceEpoch())}
    };
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QList<QHostAddress> DiscoverService::ScanHost(){
    udpsocket=new QUdpSocket();
    udpsocket->setProxy(QNetworkProxy::NoProxy);
    qDebug() << "start";
    if(!udpsocket->bind(QHostAddress::AnyIPv4,DiscoverPort,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) return QList<QHostAddress>();
    qDebug() << "connect";
    connect(udpsocket,&QUdpSocket::readyRead,this,[=](){
        qDebug() << "connected";
        QHostAddress sender;
        quint16 senderPort;
        while(udpsocket->hasPendingDatagrams()){

            QByteArray buf; buf.resize(int(udpsocket->pendingDatagramSize()));
            udpsocket->readDatagram(buf.data(), buf.size(), &sender, &senderPort);

            if(HandleUDP(buf.data(),sender,senderPort)) qDebug() << senderPort <<"收到数据"<<buf; ;

            udpinfo=buf.data();

        }
        return QList<QHostAddress>().append(sender);
    });
    return QList<QHostAddress>();
}

QString DiscoverService::getFirstLocalIPv4() {
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : addresses) {
        //qDebug()<<address;
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            !address.isLoopback() &&
            address.toString().startsWith("192.168.0")) {
            return address.toString();
        }
    }
    return "未找到IP";
}

void DiscoverService::LoopBroadcast(){
    QUdpSocket *loopsocket=new QUdpSocket();
    while(1){
        QThread::sleep(4);
        qDebug()<<"循环"<<QDateTime::currentDateTime();
        BroadcastDevice(loopsocket,BroadcastType::BROADCAST,DiscoverPort,QHostAddress(getFirstLocalIPv4()));
    }
}

bool DiscoverService::HandleUDP(const QByteArray& data,QHostAddress sender,quint16 port){
    QJsonParseError error;
    QJsonDocument json=QJsonDocument::fromJson(data,&error);
    if(error.error!=QJsonParseError::NoError)
        return false;
    QJsonObject obj=json.object();
    //emit accepted("\n收到来自"+obj["Name"].toString()+"主机:"+sender.toString()+":"+QString::number(port)+"的信息");
    //有效性判断
    //if(obj["Header"].toString()!=udpHeader||obj["Name"].toString()== DevName) return false;
    if(obj["Instance"].toString()==instanceID) return false;
    switch(obj["Method"].toInt()){
        case static_cast<int>(BroadcastType::BROADCAST):
            emit accepted("\n收到来自"+obj["Name"].toString()+"主机:"+sender.toString()+":"+QString::number(port)+"的信息---类型:广播");
            break;
        case static_cast<int>(BroadcastType::REQUEST):
            emit accepted("\n收到来自"+obj["Name"].toString()+"主机:"+sender.toString()+":"+QString::number(port)+"的信息---类型:查询");
            BroadcastDevice(new QUdpSocket(),BroadcastType::RESPONE,DiscoverPort,QHostAddress(getFirstLocalIPv4()),sender);
            break;
        case static_cast<int>(BroadcastType::RESPONE):
            emit accepted("\n收到来自"+obj["Name"].toString()+"主机:"+sender.toString()+":"+QString::number(port)+"的信息---类型:回复");
            //QString name=obj["Name"].toString();
            for (const auto& dev : AvailableDev) { // 范围 for 循环
                if (dev.name == obj["Name"].toString()||dev.ip.toString()==obj["Ip"].toString()) {       // 判断结构体字段
                    qDebug()<<"收到回复，为本机其他进程发出，示例id:"<<obj["Instance"].toString();
                    return false;
                }
            }
            AvailableDev.push_back({obj["Name"].toString()
                                    ,sender
                                    ,port
                                    ,static_cast<quint16>(obj["Port"].toString().toUShort())
                                    ,obj["Ts"].toString().toULongLong()});
            emit updateDev();
            break;
        case static_cast<int>(BroadcastType::HEARTBEAT):
            qDebug()<<"66";
            break;
        default:
            return false;
    }
    qDebug()<<obj["Header"].toString();
    return true;
}

void DiscoverService::RequestUDP(){
    BroadcastDevice(udpsocket,BroadcastType::REQUEST,DiscoverPort,QHostAddress(getFirstLocalIPv4()));
    return;
}

