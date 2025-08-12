#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "discoverservice.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
    ui->savepathLineEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    setAcceptDrops(true);

    // QThread *broadthread=new QThread();
    // DiscoverService *broadthread_body=new DiscoverService();
    // broadthread_body->moveToThread(broadthread);
    // connect(broadthread, &QThread::started, broadthread_body, &DiscoverService::LoopBroadcast);
    //broadthread->start();

    broad=new DiscoverService();
    broad->instanceID=QUuid::createUuid().toString(QUuid::WithoutBraces);
    connect(broad, &DiscoverService::accepted, this, [this](const QString& txt) {
        //qDebug() <<"accept";
        ui->textBrowser->append(txt);
    });
    broad->ScanHost();

    connect(broad,&DiscoverService::updateDev,this,[this](){
        for(const auto& dev : broad->AvailableDev){
            ui->comboBox->addItem(dev.name+"  :  "+dev.ip.toString());
        }
    });


    receivefile=new ReceiveFile(55555);
    receivefile->filepath=ui->savepathLineEdit->text();
    qDebug()<<"listening";
    connect(receivefile,&ReceiveFile::newConn,this,[this](){
        QMessageBox::information(this,"新文件","收到新发送请求");
    });
    connect(receivefile,&ReceiveFile::receivedSucc,this,[this](bool EqualhashOrnot){
        ReceiveFile *receiver=(ReceiveFile *)sender();
        QString text="文件名:"+receiver->filename+"\n大小:"+QString::number(receiver->filesize)+"\nHash:"+receiver->filehash;
        ui->textBrowser->append("接收成功"+text+(EqualhashOrnot?"\n文件一致":"\n文件不一致"));
        ui->textBrowser->append("接收文件名:"+receiver->filepath+"\n大小:"+QString::number(receiver->filesize)+"\nHash:"+receiver->filehash);
    });
    connect(receivefile,&ReceiveFile::respACK,this,[this](){
        ReceiveFile *receiver=(ReceiveFile *)sender();
        ui->textBrowser->append("当前接收"+QString::number(receiver->receivedBytes)+"原"+QString::number(receiver->filesize));
    });


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_RefreshIPButton_clicked()
{

    broad->RequestUDP();


}


void MainWindow::on_pushButton_2_clicked()
{
    //DiscoverService *broad=new DiscoverService();
    //broad->ScanHost();

    qDebug()<<broad->AvailableDev.length();
    qDebug()<<broad->AvailableDev.at(0).name;
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
    ui->textBrowser->append("文件名:"+send->filepath+"\n大小:"+QString::number(send->fileobj->size())+"\nHash:"+send->filehash);
    connect(send->tcpsocket,&QTcpSocket::connected,this,[this](){
        QTcpSocket *send=(QTcpSocket *)sender();
        ui->statuslabel->setText("连接成功");
    });
    connect(send->tcpsocket,&QTcpSocket::disconnected,this,[this](){
        //SendFile *disConn=(SendFile *)sender();
        //if(disConn->sentsize < quint64(disConn->fileobj->size()))  ui->statuslabel->setText("连接断开");
    });
    connect(send->tcpsocket, &QAbstractSocket::errorOccurred, this,[this](){
        ui->statuslabel->setText("连接断开");
    });
    connect(send,&SendFile::progressnum,this,[this](quint16 num){
        ui->progressBar->setValue(int(num));
        if(num==100) ui->statuslabel->setText("发送完毕");
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

