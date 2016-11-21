#include "protocol_manager.h"
#include "mainwindow.h"
#include "eventdialog.h"
#include "finddialog.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>
#include <QRegExp>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>

#define STR_UPDATA_IMEI "Updata the specify imei data"
#define STR_GET_LOG     "Get log"
#define STR_GET_433     "Get 433"
#define STR_GET_GSM     "Get GSM"
#define STR_GET_GPS     "Get GPS"
#define STR_GET_SETTING "Get setting"
#define STR_GET_BATTERY "Get battery"
#define STR_REBOOT      "Reboot"
#define STR_UPGRADE     "Upgrade"
#define STR_CMD_AT      "cmd AT"
#define STR_SET_SERVER  "Set device server"


QString gDefaultServer = QString("121.42.38.93:9898");//调试服务器
//QString gDefaultServer = QString("120.25.157.233:9898");//正式服务器


EventDialog *eventdialog = NULL;

QString gCurrentImeiString;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("小安宝设备控制");
    this->setMaximumSize(860,520);
    this->setMinimumSize(860,520);

    ui->lineEdit_Server->setText(gDefaultServer);

    connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slotHeaderClicked(int)));

    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket,SIGNAL(connected()),this,SLOT(slotConnected()));
    connect(tcpSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(slotDataReceived()));

    eventdialog = new EventDialog(this);
    connect(this, SIGNAL(send_daily2Eventdialog(QString)), eventdialog, SLOT(get_daily2Eventdialog(QString)));
    connect(this, SIGNAL(send_start2Eventdialog(void)), eventdialog, SLOT(get_dtart2Eventdialog(void)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("小安宝设备控制"), ".", tr("Text Files(*.txt)"));
    if(!path.isEmpty())
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, tr("小安宝设备控制"), tr("Cannot open file:\n%1").arg(path));
            return;
        }
        QTextStream stream(&file);
        QString line;
        ui->tableWidget->setRowCount(0);//clear the table
        ui->label_InProcess_GetImeiList->setText("Geting");
        while(!stream.atEnd())
        {
            line = stream.readLine(); // 一行一行读取，不包括“/n”的一行文本

            int rowNum = ui->tableWidget->rowCount();
            ui->tableWidget->setRowCount(rowNum+1);

            ui->tableWidget->setItem(rowNum, 0, new QTableWidgetItem(line));
            ui->tableWidget->setItem(rowNum, 1, new QTableWidgetItem("Details"));
            ui->tableWidget->item(rowNum, 0)->setForeground(Qt::blue);
            ui->tableWidget->item(rowNum, 1)->setForeground(Qt::blue);
        }
        ui->tableWidget->resizeColumnsToContents();
        ui->label_InProcess_GetImeiList->setText("");

        file.close();
    }
    else
    {
        QMessageBox::warning(this, tr("小安宝设备控制"), tr("You did not select any file."));
    }
}

void MainWindow::findInTableWidget(QString string)
{
    qDebug() << "findInTableWidget:" << string;

    if(ui->tableWidget->findItems(string, Qt::MatchExactly).isEmpty())
    {
        qDebug() << "failed to find:" << string;
    }
    else
    {
        qDebug() << "succeed to find:" << string;

        int column = ui->tableWidget->findItems(string, Qt::MatchContains).first()->column();
        int row = ui->tableWidget->findItems(string, Qt::MatchContains).first()->row();

        ui->tableWidget->setCurrentCell(row, column, QItemSelectionModel::Select);
    }

}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_F)
    {
        qDebug() << "get keyPressEvent Ctrl+F";

        FindDialog find(this);
        connect(&find, SIGNAL(findString(QString)), this, SLOT(findInTableWidget(QString)));
        find.exec();
    }
}

void MainWindow::uiShowConnectionStatus(bool connected)
{
    ui->pushButton_Connect->setEnabled(!connected);
    ui->lineEdit_Server->setEnabled(!connected);
    ui->pushButton_Disconnect->setEnabled(connected);
    ui->pushButton_GetImeiData->setEnabled(false);
    ui->pushButton_Get_Local_imeiData->setEnabled(false);
    ui->pushButton_UpdataImeiData->setEnabled(false);
    ui->tableWidget->setEnabled(connected);

    ui->tableWidget->setRowCount(0);
    ui->label_InProcess_GetImeiList->setText("");
    ui->label_InProcess_UpdataImeiData->setText("");

    return;
}

void MainWindow::on_pushButton_Connect_clicked()
{
    QString strServer = ui->lineEdit_Server->text();
    QRegExp regServer("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})");
    int iServerPos = regServer.indexIn(strServer);
    if(iServerPos >= 0)
    {
        qDebug() << "connect to server" << regServer.cap(1) << regServer.cap(2);
        tcpSocket->connectToHost(regServer.cap(1), regServer.cap(2).toInt(0, 10));
    }
    else
    {
        qDebug() << "connect to gDefaultServer:" << gDefaultServer;
        ui->lineEdit_Server->setText(gDefaultServer);
        tcpSocket->connectToHost(QString("120.25.157.233"), 9898);//default server
    }

    QByteArray array_header = QByteArray::fromHex("aa6601110000"); //set seq at 0xff for updata imei data loop
    tcpSocket->write(array_header);

    return;
}

void MainWindow::on_pushButton_Disconnect_clicked()
{
    tcpSocket->disconnectFromHost();

    ui->tableWidget->setRowCount(0); //clear the table
    return;
}

void MainWindow::on_pushButton_Get_Local_imeiData_clicked()
{
    ui->pushButton_GetImeiData->setEnabled(false);
    qDebug()<<"get local imei list";
    openFile();
    return;
}

void MainWindow::on_pushButton_GetImeiData_clicked()
{
    ui->pushButton_Get_Local_imeiData->setEnabled(false);
    ui->tableWidget->setRowCount(0);//clear the table
    QByteArray array_header;
    array_header = QByteArray::fromHex("aa660ccc0000");
    tcpSocket->write(array_header);
}

void MainWindow::slotConnected()
{
    qDebug() << "slotConnected";

    //modify the buttons
    uiShowConnectionStatus(true);
}

void MainWindow::slotDisconnected()
{
    qDebug() << "slotDisconnected";

    //modify the buttons
    uiShowConnectionStatus(false);
}

void MainWindow::slotHeaderClicked(int column)
{
    qDebug() << "slotHeaderClicked" << column;

    ui->tableWidget->sortByColumn(column);
}

int MainWindow::manager_login(const void *msg)
{
    const MANAGER_MSG_LOGIN_RSP *rsp = (const MANAGER_MSG_LOGIN_RSP *)msg;
    if(ntohs(rsp->length) != sizeof(MANAGER_MSG_LOGIN_REQ) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("login message length not enough");
        return -1;
    }

    qDebug("get manager login response");

    ui->pushButton_GetImeiData->setEnabled(true);
    ui->pushButton_UpdataImeiData->setEnabled(true);
    ui->pushButton_Get_Local_imeiData->setEnabled(true);
    return 0;
}

void MainWindow::uiShowImeiData(const char *imei, char online_offline, int version, int timestamp, float longitude, float latitude, char speed, short course)
{
    //qDebug("%d,%d,%f,%f,%d,%d",online_offline, timestamp, longitude, latitude, speed, course);
    QDateTime dataTime = QDateTime::fromTime_t(timestamp);
    QString dataTimeString = dataTime.toString("yyyy.MM.dd HH:mm:ss");

    if(ui->tableWidget->findItems(imei, Qt::MatchExactly).isEmpty())
    {
        qDebug() << "can't find items"  << imei;
    }
    else
    {
        qDebug() << "find items" << imei;

        int rowNum = ui->tableWidget->findItems(imei, Qt::MatchEndsWith).first()->row();

        if(online_offline == 1)
        {
            ui->tableWidget->setItem(rowNum, 2, new QTableWidgetItem("online"));
            ui->tableWidget->item(rowNum, 2)->setForeground(Qt::green);
        }
        else if(online_offline == 2)
        {
            ui->tableWidget->setItem(rowNum, 2, new QTableWidgetItem("offline"));
            ui->tableWidget->item(rowNum, 2)->setForeground(Qt::red);
        }

        int version_a = version / 65536;
        int version_b = (version % 65536) / 256;
        int version_c = version % 256;
        ui->tableWidget->setItem(rowNum, 3, new QTableWidgetItem(QString("%1.%2.%3").arg(version_a).arg(version_b).arg(version_c)));
        ui->tableWidget->setItem(rowNum, 4, new QTableWidgetItem(dataTimeString));
        ui->tableWidget->setItem(rowNum, 5, new QTableWidgetItem(QString::number(longitude, 'f', 6)));
        ui->tableWidget->setItem(rowNum, 6, new QTableWidgetItem(QString::number(latitude, 'f', 6)));
        ui->tableWidget->setItem(rowNum, 7, new QTableWidgetItem(QString("%1").arg((int)speed)));
        ui->tableWidget->setItem(rowNum, 8, new QTableWidgetItem(QString("%1").arg(course)));
        ui->tableWidget->resizeColumnsToContents();
    }

    return;
}

int MainWindow::manager_imeiData(const void *msg)
{
    const MANAGER_MSG_IMEI_DATA_RSP *rsp = (const MANAGER_MSG_IMEI_DATA_RSP *)msg;
    if(ntohs(rsp->header.length) != sizeof(MANAGER_MSG_IMEI_DATA_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("imei data message length not enough");
        return -1;
    }

    char imei[MANAGER_MAX_IMEI_LENGTH + 1];
    memcpy(imei, rsp->imei_data.IMEI, MANAGER_MAX_IMEI_LENGTH);
    imei[MANAGER_MAX_IMEI_LENGTH] = '\0'; //add '\0' for string operaton

    char online_offline = rsp->imei_data.online_offline;
    int version = rsp->imei_data.version;
    int timestamp = rsp->imei_data.gps.timestamp;
    float longitude = rsp->imei_data.gps.longitude;
    float latitude = rsp->imei_data.gps.latitude;
    char speed = rsp->imei_data.gps.speed;
    short course = rsp->imei_data.gps.course;

    uiShowImeiData(imei, online_offline, version, timestamp, longitude, latitude, speed, course);

    //qDebug() << ntohs(rsp->header.signature) << (unsigned int)(rsp->header.cmd) << (unsigned int)(rsp->header.seq) << ntohs(rsp->header.length);
    if(rsp->header.seq == (char)0xff)
    {
        //updata imei data loop
        int rowNum = ui->tableWidget->findItems(imei, Qt::MatchExactly).first()->row();
        if(rowNum + 1 < ui->tableWidget->rowCount())
        {
            UpdataImeiDataWithRow(rowNum + 1);
        }
        else
        {
            ui->label_InProcess_UpdataImeiData->setText("");
        }
    }
    return 0;
}

int MainWindow::manager_getLog(const void *msg)
{
    const MANAGER_MSG_GET_LOG_RSP *rsp = (const MANAGER_MSG_GET_LOG_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_GET_LOG_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getLog message length not enough");
        return -1;
    }

    qDebug("get manager getLog response, %s", rsp->data);

    switch(QMessageBox::information(this, "information", QString("Get Log: %1").arg(rsp->data), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape ))
    {
        case QMessageBox::Ok:
            qDebug() << "QMessageBox::Ok";
            break;
        case QMessageBox::Cancel:
            qDebug() << "QMessageBox::Cancel";
            break;
        default:
            break;
    }
    return 0;
}

int MainWindow::manager_get433(const void *msg)
{
    const MANAGER_MSG_GET_433_RSP *rsp = (const MANAGER_MSG_GET_433_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_GET_433_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("get433 message length not enough");
        return -1;
    }

    qDebug("get manager get433 response, %s", rsp->data);

    switch(QMessageBox::information(this, "information", QString("Get 433: %1").arg(rsp->data), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape ))
    {
        case QMessageBox::Ok:
            qDebug() << "QMessageBox::Ok";
            break;
        case QMessageBox::Cancel:
            qDebug() << "QMessageBox::Cancel";
            break;
        default:
            break;
    }
    return 0;
}

int MainWindow::manager_getGSM(const void *msg)
{
    const MANAGER_MSG_GET_GSM_RSP *rsp = (const MANAGER_MSG_GET_GSM_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_GET_GSM_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getGSM message length not enough");
        return -1;
    }

    qDebug("get manager getGSM response, %s", rsp->data);

    switch(QMessageBox::information(this, "information", QString("Get GSM: %1").arg(rsp->data), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape ))
    {
        case QMessageBox::Ok:
            qDebug() << "QMessageBox::Ok";
            break;
        case QMessageBox::Cancel:
            qDebug() << "QMessageBox::Cancel";
            break;
        default:
            break;
    }
    return 0;
}

int MainWindow::manager_getGPS(const void *msg)
{
    const MANAGER_MSG_GET_GPS_RSP *rsp = (const MANAGER_MSG_GET_GPS_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_GET_GPS_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getGPS message length not enough");
        return -1;
    }

    qDebug("get manager getGPS response, %s", rsp->data);

    switch(QMessageBox::information(this, "information", QString("Get GPS: %1").arg(rsp->data), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape ))
    {
        case QMessageBox::Ok:
            qDebug() << "QMessageBox::Ok";
            break;
        case QMessageBox::Cancel:
            qDebug() << "QMessageBox::Cancel";
            break;
        default:
            break;
    }
    return 0;
}

int MainWindow::manager_getSetting(const void *msg)
{
    const MANAGER_MSG_GET_SETTING_RSP *rsp = (const MANAGER_MSG_GET_SETTING_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_GET_SETTING_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getSetting message length not enough");
        return -1;
    }

    qDebug("get manager getSetting response, %s", rsp->data);

    switch(QMessageBox::information(this, "information", QString("Get Setting: %1").arg(rsp->data), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape ))
    {
        case QMessageBox::Ok:
            qDebug() << "QMessageBox::Ok";
            break;
        case QMessageBox::Cancel:
            qDebug() << "QMessageBox::Cancel";
            break;
        default:
            break;
    }
    return 0;
}

int MainWindow::manager_getBattery(const void *msg)
{
    const MANAGER_MSG_GET_BATTERY_RSP *rsp = (const MANAGER_MSG_GET_BATTERY_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_GET_BATTERY_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getBattery message length not enough");
        return -1;
    }

    qDebug("get manager getBattery response, %s", rsp->data);

    switch(QMessageBox::information(this, "information", QString("Get Battery: %1").arg(rsp->data), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape ))
    {
        case QMessageBox::Ok:
            qDebug() << "QMessageBox::Ok";
            break;
        case QMessageBox::Cancel:
            qDebug() << "QMessageBox::Cancel";
            break;
        default:
            break;
    }
    return 0;
}

int MainWindow::manager_getAT(const void *msg)
{
    const MANAGER_MSG_GET_BATTERY_RSP *rsp = (const MANAGER_MSG_GET_BATTERY_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_GET_BATTERY_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getAT message length not enough");
        return -1;
    }

    qDebug("get manager getAT response, %s", rsp->data);

    switch(QMessageBox::information(this, "information", QString("Get AT: %1").arg(rsp->data), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape ))
    {
        case QMessageBox::Ok:
            qDebug() << "QMessageBox::Ok";
            break;
        case QMessageBox::Cancel:
            qDebug() << "QMessageBox::Cancel";
            break;
        default:
            break;
    }
    return 0;
}

int MainWindow::manager_getOneDaily(const void *msg)
{
    const MANAGER_MSG_IMEI_DAILY_RSP *rsp = (const MANAGER_MSG_IMEI_DAILY_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_IMEI_DAILY_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getNULL message length not enough");
        return -1;
    }
    char data[128] = {0};
    memcpy(data, rsp->data, 128);

    send_daily2Eventdialog(data);

    return 0;
}

int MainWindow::manager_setServerRsp(const void *msg)
{
    QMessageBox::information(this, "Information", QString("Set server OK, reboot it!"), QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape );
    return 0;
}

int MainWindow::manager_getImeiData(const void *msg)
{
    const MANAGER_MSG_IMEI_DATA_RSP *rsp = (const MANAGER_MSG_IMEI_DATA_RSP *)msg;
    if(ntohs(rsp->header.length) < sizeof(MANAGER_MSG_IMEI_DATA_RSP) - MANAGER_MSG_HEADER_LEN)
    {
        qDebug("getNULL message length not enough");
        return -1;
    }
    qDebug("get data(%s)", rsp->imei_data.IMEI);

    int rowNum = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(rowNum+1);

    char imei[16] = {0};
    memcpy(imei, rsp->imei_data.IMEI, 15);
    ui->tableWidget->setItem(rowNum, 0, new QTableWidgetItem(imei));
    ui->tableWidget->item(rowNum, 0)->setForeground(Qt::blue);

    ui->tableWidget->setItem(rowNum, 1, new QTableWidgetItem("Details"));
    ui->tableWidget->item(rowNum, 1)->setForeground(Qt::blue);

    if(rsp->imei_data.online_offline == 1)
    {
        ui->tableWidget->setItem(rowNum, 2, new QTableWidgetItem("online"));
        ui->tableWidget->item(rowNum, 2)->setForeground(Qt::green);
    }
    else if(rsp->imei_data.online_offline == 2)
    {
        ui->tableWidget->setItem(rowNum, 2, new QTableWidgetItem("offline"));
        ui->tableWidget->item(rowNum, 2)->setForeground(Qt::red);
    }

    int version = ntohl(rsp->imei_data.version);
    int version_a = version / 65536;
    int version_b = (version % 65536) / 256;
    int version_c = version % 256;
    ui->tableWidget->setItem(rowNum, 3, new QTableWidgetItem(QString("%1.%2.%3").arg(version_a).arg(version_b).arg(version_c)));

    int timestamp = ntohl(rsp->imei_data.gps.timestamp);
    QDateTime dataTime = QDateTime::fromTime_t(timestamp);
    QString dataTimeString = dataTime.toString("yyyy.MM.dd HH:mm:ss");
    ui->tableWidget->setItem(rowNum, 4, new QTableWidgetItem(dataTimeString));

    ui->tableWidget->setItem(rowNum, 5, new QTableWidgetItem(QString::number(rsp->imei_data.gps.longitude, 'f', 6)));
    ui->tableWidget->setItem(rowNum, 6, new QTableWidgetItem(QString::number(rsp->imei_data.gps.latitude, 'f', 6)));
    ui->tableWidget->setItem(rowNum, 7, new QTableWidgetItem(QString("%1").arg((int)rsp->imei_data.gps.speed)));

    short course = ntohs(rsp->imei_data.gps.course);
    ui->tableWidget->setItem(rowNum, 8, new QTableWidgetItem(QString("%1").arg(course)));

    ui->tableWidget->setItem(rowNum, 9, new QTableWidgetItem(QString("0")));
    ui->tableWidget->resizeColumnsToContents();

    return 0;
}

int MainWindow::handle_one_msg(const void *m)
{
    const MANAGER_MSG_HEADER *msg = (const MANAGER_MSG_HEADER *)m;
    switch(msg->cmd)
    {
        case MANAGER_CMD_LOGIN:
            return manager_login(msg);

        case MANAGER_CMD_IMEI_DATA:
            return manager_imeiData(msg);

        case MANAGER_CMD_GET_LOG:
            return manager_getLog(msg);

        case MANAGER_CMD_GET_433:
            return manager_get433(msg);

        case MANAGER_CMD_GET_GSM:
            return manager_getGSM(msg);

        case MANAGER_CMD_GET_GPS:
            return manager_getGPS(msg);

        case MANAGER_CMD_GET_SETTING:
            return manager_getSetting(msg);

        case MANAGER_CMD_GET_BATTERY:
            return manager_getBattery(msg);

        case MANAGER_CMD_GET_AT:
            return manager_getAT(msg);

        case MANAGER_CMD_GET_IMEIDATA:
            return manager_getImeiData(msg);

        case MANAGER_CMD_GET_IMEIDAILY:
            return manager_getOneDaily(msg);

        case MANAGER_CMD_SET_SERVER:
            return manager_setServerRsp(msg);



        default:
            return -1;
    }
}

static QByteArray datagram;

int MainWindow::handle_manager_msg(void)
{
    MANAGER_MSG_HEADER *msg = (MANAGER_MSG_HEADER *)datagram.data();

    if(datagram.length() < MANAGER_MSG_HEADER_LEN)
    {
        qDebug("message length not enough: %zu(at least(%zu))", datagram.length(), MANAGER_MSG_HEADER_LEN);

        return -1;
    }
    size_t leftLen = datagram.length();
    while(leftLen >= ntohs(msg->length) + MANAGER_MSG_HEADER_LEN)
    {
        const unsigned char *status = (const unsigned char *)(&(msg->signature));
        if((status[0] != 0xaa) || (status[1] != 0x66))
        {
            qDebug("receive message header signature error:%x", (unsigned)ntohs(msg->signature));
        }
        else
        {
            handle_one_msg(msg);
        }

        leftLen = leftLen - MANAGER_MSG_HEADER_LEN - ntohs(msg->length);
        datagram.remove(0,MANAGER_MSG_HEADER_LEN + ntohs(msg->length));
        msg = (MANAGER_MSG_HEADER *)datagram.data();
    }
    return 0;
}

void MainWindow::slotDataReceived()
{

    while(tcpSocket->bytesAvailable() > 0)
    {
        datagram.append(tcpSocket->readAll());
        handle_manager_msg();
    }

    return;
}

void MainWindow::slotTableMenuAction(QAction *action)
{
    qDebug() << "slotTableMenuAction" << action->text();
    QByteArray array_header;
    QByteArray array_imei = QByteArray::fromHex(gCurrentImeiString.toLatin1().toHex());

    if(action->text() == STR_UPDATA_IMEI)
    {
        array_header = QByteArray::fromHex("aa660222000f");
    }
    else if(action->text() == STR_GET_LOG)
    {
        array_header = QByteArray::fromHex("aa660333000f");
    }
    else if(action->text() == STR_GET_433)
    {
        array_header = QByteArray::fromHex("aa660444000f");
    }
    else if(action->text() == STR_GET_GSM)
    {
        array_header = QByteArray::fromHex("aa660555000f");
    }
    else if(action->text() == STR_GET_GPS)
    {
        array_header = QByteArray::fromHex("aa660666000f");
    }
    else if(action->text() == STR_GET_SETTING)
    {
        array_header = QByteArray::fromHex("aa660777000f");
    }
    else if(action->text() == STR_GET_BATTERY)
    {
        array_header = QByteArray::fromHex("aa660888000f");
    }
    else if(action->text() == STR_SET_SERVER)
    {
        bool isOK;
        QString strServer = QInputDialog::getText(NULL,"Set server IP","server:", QLineEdit::Normal,"121.42.38.93:9880",&isOK);
        if(isOK)
        {
            QRegExp regServer("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})");
            int iServerPos = regServer.indexIn(strServer);
            if(iServerPos >= 0)
            {
                array_header = QByteArray::fromHex("aa660eee000f");
                tcpSocket->write(array_header + array_imei + strServer.toUtf8()+"\r");
            }
            else
            {
                QMessageBox::information(this, "Warning", QString("Please input one right server IP!"), QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape );

            }
        }
        return;
    }
    else if(action->text() == STR_REBOOT)
    {
        switch(QMessageBox::question(this, "warning", QString("Are you sure to reboot the device(%1)").arg(gCurrentImeiString), QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape ))
        {
            case QMessageBox::Yes:
                qDebug() << "QMessageBox::Yes";
                array_header = QByteArray::fromHex("aa660999000f");
                break;
            case QMessageBox::No:
                qDebug() << "QMessageBox::No";
                return;
            default:
                break;
        }
    }
    else if(action->text() == STR_UPGRADE)
    {
        switch(QMessageBox::question(this, "warning", QString("Are you sure to upgrade the device(%1)").arg(gCurrentImeiString), QMessageBox::Yes | QMessageBox::Default, QMessageBox::No | QMessageBox::Escape ))
        {
            case QMessageBox::Yes:
                qDebug() << "QMessageBox::Yes";
                array_header = QByteArray::fromHex("aa660aaa000f");
                break;
            case QMessageBox::No:
                qDebug() << "QMessageBox::No";
                return;
            default:
                break;
        }
    }
    else if(action->text() == STR_CMD_AT)
    {
        bool isOK;
        QString text = QInputDialog::getText(NULL,"AT_CMD","AT Command", QLineEdit::Normal,NULL,&isOK);
        if(isOK)
        {
            array_header = QByteArray::fromHex("aa660bbb000f");
            tcpSocket->write(array_header + array_imei + text.toUtf8()+"\r");
            return;
        }
        else
        {
            return;
        }
    }

    tcpSocket->write(array_header + array_imei);
}

void MainWindow::on_tableWidget_cellDoubleClicked(int row, int column)
{
    gCurrentImeiString = ui->tableWidget->item(row, 0)->text();
    qDebug() << "on_tableWidget_cellDoubleClicked:" << row << column << gCurrentImeiString;

    if(column == 0) //get imei data
    {
        pTableMenu = new QMenu(ui->tableWidget);
        pTableMenu->addAction(STR_UPDATA_IMEI);
        pTableMenu->addSeparator();
        pTableMenu->addAction(STR_GET_LOG);
        pTableMenu->addAction(STR_GET_433);
        pTableMenu->addAction(STR_GET_GSM);
        pTableMenu->addAction(STR_GET_GPS);
        pTableMenu->addAction(STR_GET_SETTING);
        pTableMenu->addAction(STR_GET_BATTERY);
        pTableMenu->addAction(STR_CMD_AT);
        pTableMenu->addSeparator();
        pTableMenu->addAction(STR_SET_SERVER);
        pTableMenu->addAction(STR_REBOOT);
        pTableMenu->addAction(STR_UPGRADE);


        connect(pTableMenu, SIGNAL(triggered(QAction *)), this, SLOT(slotTableMenuAction(QAction *)));
        pTableMenu->exec(QCursor::pos());
    }

    if(column == 1) //get event
    {
        send_start2Eventdialog();
        QByteArray array_header = QByteArray::fromHex("aa660ddd000f"); //set seq at 0xff for updata imei data loop
        QByteArray array_imei = QByteArray::fromHex(gCurrentImeiString.toLatin1().toHex());
        tcpSocket->write(array_header + array_imei);

        eventdialog->exec();
    }
}

void MainWindow::UpdataImeiDataWithRow(int row)
{
    QString imeiString = ui->tableWidget->item(row, 0)->text(); //get imei string
    qDebug() << "UpdataImeiDataWithRow" << imeiString;

    QByteArray array_header = QByteArray::fromHex("aa6602ff000f"); //set seq at 0xff for updata imei data loop
    QByteArray array_imei = QByteArray::fromHex(imeiString.toLatin1().toHex());

    tcpSocket->write(array_header + array_imei);

}

void MainWindow::on_pushButton_UpdataImeiData_clicked()
{
    if(ui->tableWidget->rowCount() != 0)
    {
        UpdataImeiDataWithRow(0);
        ui->label_InProcess_UpdataImeiData->setText("Updating");
    }
}
