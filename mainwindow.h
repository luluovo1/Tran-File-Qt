#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QDebug>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QStandardPaths>
#include "discoverservice.h"
#include "sendfile.h"
#include "receivefile.h"
#include "toast.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void InitReceiver();
    void InitBroadcast();


protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:

    void on_RefreshIPButton_clicked();

    void on_BrowserButton_clicked();

    void on_PortLineEdit_textEdited(const QString &arg1);

    void on_comboBox_currentIndexChanged(int index);

    void on_Sendbutton_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_receiveportLineEdit_editingFinished();

private:
    Ui::MainWindow *ui;

    DiscoverService *broad = nullptr;      // 🔧 显式初始化防止悬空指针

    SendFile *sendfile = nullptr;          // 🔧 显式初始化防止悬空指针
    ReceiveFile *receivefile = nullptr;    // 🔧 显式初始化防止悬空指针

signals:

};
#endif // MAINWINDOW_H
