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
    ui->tabWidget->setCurrentIndex(0);
    ui->savepathLineEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    setAcceptDrops(true);

    InitBroadcast();
    receivefile=new ReceiveFile();
    InitReceiver();

}


void MainWindow::InitBroadcast(){
    // QThread *broadthread=new QThread();
    // DiscoverService *broadthread_body=new DiscoverService();
    // broadthread_body->moveToThread(broadthread);
    // connect(broadthread, &QThread::started, broadthread_body, &DiscoverService::LoopBroadcast);
    //broadthread->start();

    broad=new DiscoverService();
    broad->instanceID=QUuid::createUuid().toString(QUuid::WithoutBraces);
    connect(broad, &DiscoverService::accepted, this, [this](const QString& txt) {
        ui->textBrowser->append(txt);
    });
    broad->ScanHost();

    connect(broad,&DiscoverService::updateDev,this,[this](){
        for(const auto& dev : broad->AvailableDev){
            ui->comboBox->addItem(dev.name+"  :  "+dev.ip.toString());
        }
    });
}


void MainWindow::InitReceiver(){


    receivefile->filepath=ui->savepathLineEdit->text();
    qDebug()<<"listening";
    connect(receivefile,&ReceiveFile::newConn,this,[this](const QHostAddress& sourceIP,const quint16 sourcePort){
        ui->textBrowser->append("<font color='green'>接收端:收到来自主机:"+sourceIP.toString()+" 端口:"+QString::number(sourcePort)+"的文件---开始接收</font>");
        Toast::showBottomRight("<font color='white'>接收端:收到文件\n主机:"+sourceIP.toString()+"\n端口:"+QString::number(sourcePort),5000);
    });
    connect(receivefile,&ReceiveFile::receivedSucc,this,[this](bool EqualhashOrnot){
        ReceiveFile *receiver=(ReceiveFile *)sender();
        QString isEqual=EqualhashOrnot?"一致,成功!":"不一致,失败!";
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
    delete ui;
}

void MainWindow::on_RefreshIPButton_clicked()
{
    broad->AvailableDev.clear();
    ui->textBrowser->clear();
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
    ui->ReceiveIPLineEdit->setText(broad->AvailableDev.at(index).ip.toString());
}


void MainWindow::on_Sendbutton_clicked()
{
    QString fileName=ui->FileNameLineEdit->text();
    if(!QFileInfo(fileName).isFile()) {
        QMessageBox::warning(this,"Failed","检查输入文件");
        return;
    }
    QHostAddress target=QHostAddress(ui->ReceiveIPLineEdit->text());
    quint16 port=ui->PortLineEdit->text().toUShort();
    SendFile *send=new SendFile(target,port,fileName);
    qDebug()<<"发送："<<fileName<<target<<port;
    ui->textBrowser->append("<font color='blue'>发送->文件名:"+send->filepath+" 大小:"+QString::number(send->fileobj->size())+"\nHash:"+send->filehash+"</font>");
    //进度条下方的状态信息
    connect(send->tcpsocket,&QTcpSocket::connected,this,[this](){
        QTcpSocket *send=(QTcpSocket *)sender();
        ui->statuslabel->setText("连接成功");
    });

    //绑定错误以及端口连接
    connect(send->tcpsocket,&QTcpSocket::disconnected,this,[this](){
        ui->textBrowser->append("<font color='blue'>发送端:当前连接已断开</font>");
    });
    connect(send->tcpsocket, &QAbstractSocket::errorOccurred, this,[this](){
        ui->statuslabel->setText("<font color='red'>发生异常</font>");
    });

    //进度条实现，信号由每次发送端写入tcp提供
    connect(send,&SendFile::progressnum,this,[this](quint16 num){
        ui->progressBar->setValue(int(num));
        if(num==100) ui->statuslabel->setText("<font color='green'>发送完毕</font>");
    });

    //绑定接收端返回的ACK信号和哈希，提供给发送端
    connect(send,&SendFile::ACKreceiver,this,[this](bool isReceived,const QString& verifyhash){
        SendFile *sendobj=(SendFile *)sender();
        QString title;
        QString text;
        if(isReceived) {
            title="发送端：成功";
            text="发送完毕,收到ACK响应\n源 Hash:"+sendobj->filehash+"\n输出 Hash:"+verifyhash+"\n"+((sendobj->filehash==verifyhash)?"对比一致,成功":"文件不一致");
        }else{
            title="发送端：失败";
            text="发送完毕,但收到ACK响应,文件可能发送失败";
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

    send->TCPconnect();
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




