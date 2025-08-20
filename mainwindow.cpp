#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "discoverservice.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //初始化
    ui->setupUi(this);
    this->setWindowTitle("梁宠文件传输");
    //ui->tabWidget->setCurrentIndex(0);
    ui->savepathLineEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    setAcceptDrops(true);

    // 🔧 安全初始化成员变量
    broad = nullptr;
    sendfile = nullptr;
    receivefile = nullptr;
    qDebug() << "start";
    // 🔧 添加异常保护
    try {
        InitBroadcast();
        receivefile=new ReceiveFile(this); // 设置parent
        InitReceiver();
    } catch (...) {
        qDebug() << "init failed MainWindow Constructor";
    }

}


void MainWindow::InitBroadcast(){
    // QThread *broadthread=new QThread();
    // DiscoverService *broadthread_body=new DiscoverService();
    // broadthread_body->moveToThread(broadthread);
    // connect(broadthread, &QThread::started, broadthread_body, &DiscoverService::LoopBroadcast);
    //broadthread->start();

    try {
        broad=new DiscoverService(this); // 🔧 设置parent
        broad->instanceID=QUuid::createUuid().toString(QUuid::WithoutBraces);
        
        connect(broad, &DiscoverService::accepted, this, [this](const QString& txt) {
            ui->textBrowser->append(txt);
        });
        qDebug() << "1";
        broad->ScanHost();
        qDebug() << "2";
        connect(broad,&DiscoverService::updateDev,this,[this](){
            // 🔧 添加安全检查
            if (!broad || broad->AvailableDev.isEmpty()) return;
            for(const auto& dev : broad->AvailableDev){
                ui->comboBox->addItem(dev.name+"  :  "+dev.ip.toString());
            }
        });
    } catch (...) {
        qDebug() << "InitBroadcast 异常，设置broad为nullptr";
        broad = nullptr;
    }
}


void MainWindow::InitReceiver(){


    receivefile->filepath=ui->savepathLineEdit->text();
    qDebug()<<"listening";
    connect(receivefile,&ReceiveFile::newConn,this,[this](const QHostAddress& sourceIP,const quint16 sourcePort){
        ui->textBrowser->append("<font color='green'>接收端:收到来自主机:"+sourceIP.toString()+" 端口:"+QString::number(sourcePort)+"的文件---开始接收</font>");
        Toast::showBottomRight("<font color='white'>接收端:收到文件\n主机:"+sourceIP.toString()+"\n端口:"+QString::number(sourcePort),5000);
    });
    connect(receivefile,&ReceiveFile::receivedSucc,this,[this](bool EqualhashOrnot){
        qDebug()<<EqualhashOrnot;
        ReceiveFile *receiver=(ReceiveFile *)sender();
        QString isEqual=EqualhashOrnot?"一致,成功!":"不一致,失败!";
        qDebug()<<isEqual;
        QString text="\n接收端:文件名:"+receiver->filename+"大小:"+QString::number(receiver->filesize)+"\nHash:"+receiver->filehash+"\n文件";
        ui->textBrowser->append("<font color='"+QString(EqualhashOrnot?"green":"red")+"'>接收文件成功"+text+isEqual+"</font>");
        ui->textBrowser->append("<font color='green'>接收端:当前连接已断开</font>");
    });
    connect(receivefile,&ReceiveFile::respACK,this,[this](){
        ReceiveFile *receiver=(ReceiveFile *)sender();
        ui->textBrowser->append("<font color='green'>接收端:当前接收字节:"+QString::number(receiver->receivedBytes)+" 总大小:"+QString::number(receiver->filesize)+"</font>");
    });
    qDebug()<<ui->receiveportLineEdit->text().toUShort();
    if(!receivefile->startlisten(55555)) QMessageBox::warning(this,"接收器异常！","接收器监听异常,如需接收文件请更换文件接收端口");
}

MainWindow::~MainWindow()
{
    // ✅ 改进：确保所有资源正确清理
    if (broad) {
        broad->deleteLater();
    }
    
    if (sendfile) {
        sendfile->deleteLater();
    }
    
    if (receivefile) {
        receivefile->closeall();
        receivefile->deleteLater();
    }
    
    delete ui;
}

void MainWindow::on_RefreshIPButton_clicked()
{
    // 🔧 添加空指针检查
    if (!broad) {
        qDebug() << "广播服务未初始化，无法刷新设备";
        QMessageBox::warning(this, "警告", "设备发现服务未正常启动，请重启程序");
        return;
    }
    
    broad->AvailableDev.clear();
    ui->textBrowser->clear();
    ui->comboBox->clear(); // 🔧 清空下拉框避免残留数据
    broad->RequestUDP();
}


void MainWindow::on_BrowserButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,                          // 父窗口（当前窗口）
        tr("选择文件"),                 // 对话框标题
        QDir::homePath(),              // 默认目录（用户主目录）
        tr("所有文件 (*);;文本文件 (*.txt);;图片 (*.png *.jpg)") // 文件过滤器
        );
    qDebug()<<fileName;
    if(!fileName.isEmpty()) ui->FileNameLineEdit->setText(fileName);
    //new SendFile(QHostAddress(),12,fileName);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event){
    if (event->mimeData()->hasUrls()) {  // 检查拖拽内容是否包含URL（文件路径）
        event->acceptProposedAction();   // 接受拖拽操作
    } else {
        event->ignore();                // 忽略非文件拖拽
    }
}

void MainWindow::dropEvent(QDropEvent *event){
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls(); // 获取拖入的文件URL列表
        QString filePath = urlList.first().toLocalFile(); // 取第一个文件的本地路径
        qDebug()<<filePath;
        ui->FileNameLineEdit->setText(filePath);            // 显示路径到LineEdit
        event->acceptProposedAction();          // 确认操作完成
    }
}

void MainWindow::closeEvent(QCloseEvent *event){
    qDebug()<<"主动关闭";
    event->accept();
}

void MainWindow::on_PortLineEdit_textEdited(const QString &arg1)
{
    int val=arg1.toInt();
    if(val<0||val>65535) ui->PortLineEdit->setText("12123");
}


void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    // 🔧 添加空指针和有效性检查
    if (!broad) {
        qDebug() << "广播服务未初始化";
        ui->ReceiveIPLineEdit->setText("127.0.0.1");
        return;
    }
    
    if (broad->AvailableDev.isEmpty()) {
        qDebug() << "设备列表为空";
        ui->ReceiveIPLineEdit->setText("127.0.0.1");
        return;
    }
    
    // ✅ 添加：边界检查防止崩溃
    if (index >= 0 && index < broad->AvailableDev.size()) {
        ui->ReceiveIPLineEdit->setText(broad->AvailableDev.at(index).ip.toString());
    } else {
        qDebug() << "Invalid device index:" << index << "Available devices:" << broad->AvailableDev.size();
        // ✅ 设置默认IP
        ui->ReceiveIPLineEdit->setText("127.0.0.1");
    }
}


void MainWindow::on_Sendbutton_clicked()
{
    qDebug()<<"0";
    QString fileName=ui->FileNameLineEdit->text();
    if(!QFileInfo(fileName).isFile()) {
        QMessageBox::warning(this,"Failed","检查输入文件");
        return;
    }
    
    // ✅ 添加：清理之前的发送任务
    if (sendfile) {
        qDebug()<<"2";
        if (sendfile->tcpsocket && sendfile->tcpsocket->state() != QAbstractSocket::UnconnectedState) {
            QMessageBox::information(this, "提示", "当前有文件正在传输中，请等待完成");
            qDebug()<<"3";
            return;
        }
        
        // 🔧 断开所有信号连接，防止悬空指针回调
        disconnect(sendfile, nullptr, this, nullptr);
        if (sendfile->tcpsocket) {
            disconnect(sendfile->tcpsocket, nullptr, this, nullptr);
        }
        
        // 🔧 正确清理旧对象
        sendfile->deleteLater();
        sendfile = nullptr;
    }
    qDebug()<<"1";
    QHostAddress target=QHostAddress(ui->ReceiveIPLineEdit->text());
    quint16 port=ui->PortLineEdit->text().toUShort();
    // ✅ 修复：使用成员变量并设置parent
    sendfile = new SendFile(target,port,fileName,this);
    qDebug()<<"发送："<<fileName<<target<<port;
    
    // 🔧 添加文件对象有效性检查
    if (!sendfile || !sendfile->fileobj || !sendfile->fileobj->exists()) {
        QMessageBox::critical(this, "错误", "无法打开选择的文件: " + fileName);
        if (sendfile) {
            sendfile->deleteLater();
            sendfile = nullptr;
        }
        return;
    }
    
    ui->textBrowser->append("<font color='blue'>发送->文件名:"+sendfile->filepath+" 大小:"+QString::number(sendfile->fileobj->size())+"\nHash:"+sendfile->filehash+"</font>");
    //进度条下方的状态信息
    connect(sendfile->tcpsocket,&QTcpSocket::connected,this,[this](){
        QTcpSocket *send=(QTcpSocket *)sender();
        ui->statuslabel->setText("连接成功");
    });

    //绑定错误以及端口连接
    connect(sendfile->tcpsocket,&QTcpSocket::disconnected,this,[this](){
        ui->textBrowser->append("<font color='blue'>发送端:当前连接已断开</font>");
    });
    connect(sendfile->tcpsocket, &QAbstractSocket::errorOccurred, this,[this](QAbstractSocket::SocketError error){
        ui->statuslabel->setText("<font color='red'>网络错误: " + sendfile->tcpsocket->errorString() + "</font>");
        qDebug() << "Socket error:" << error << sendfile->tcpsocket->errorString();
    });

    //进度条实现，信号由每次发送端写入tcp提供
    connect(sendfile,&SendFile::progressnum,this,[this](quint16 num){
        ui->progressBar->setValue(int(num));
        if(num==100) ui->statuslabel->setText("<font color='green'>发送完毕</font>");
    });

    //绑定接收端返回的ACK信号和哈希，提供给发送端
    connect(sendfile,&SendFile::ACKreceiver,this,[this](bool isReceived,const QString& verifyhash){
        SendFile *sendobj=(SendFile *)sender();
        QString title;
        QString text;
        if(isReceived) {
            title="发送端：成功";
            text="发送完毕,收到ACK响应\n源 Hash:"+sendobj->filehash+"\n输出 Hash:"+verifyhash+"\n"+((sendobj->filehash==verifyhash)?"对比一致,成功":"文件不一致");
        }else{
            title="发送端：失败";
            text="发送完毕,但未收到ACK响应,文件可能发送失败";
        }
        QMessageBox *msgBox = new QMessageBox(
            QMessageBox::Information,
            title,
            text,
            QMessageBox::Ok,
            QApplication::activeWindow() // 获取当前活动窗口作为父对象
            );
        msgBox->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动销毁
        msgBox->setWindowModality(Qt::NonModal);
        msgBox->show();
    });

    sendfile->TCPconnect();
}


void MainWindow::on_tabWidget_currentChanged(int index)
{
    switch(index){
    case 0://发送
        // receivefile=nullptr;
        break;
    case 1://接收
        break;
    }
}


void MainWindow::on_receiveportLineEdit_editingFinished()
{

    if(ui->receiveportLineEdit->text().toInt()>65534) ui->receiveportLineEdit->setText("65534");
    receivefile->tcpserver->close();
    InitReceiver();
}


