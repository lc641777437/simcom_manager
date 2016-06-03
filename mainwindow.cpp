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



//QString gDefaultServer = QString("121.42.38.93:9898");//调试服务器
QString gDefaultServer = QString("120.25.157.233:9898");//正式服务器
QString gDefaultMysql = QString("120.25.157.233:3306");
QString gCurrentImeiString;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->lineEdit_Server->setText(gDefaultServer);
    ui->lineEdit_Mysql->setText(gDefaultMysql);

    connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slotHeaderClicked(int)));

    data_base = QSqlDatabase::addDatabase("QMYSQL"); //add mysql driver
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::findInTableWidget(QString string)
{
    qDebug() << "findInTableWidget:" << string;

    if(ui->tableWidget->findItems(string, Qt::MatchContains).isEmpty())
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
        qDebug() << "get keyPressEvent ctrl+f";

        FindDialog find(this);
        connect(&find, SIGNAL(findString(QString)), this, SLOT(findInTableWidget(QString)));
        find.exec();
    }
}

void MainWindow::uiShowConnectionStatus(bool connected)
{
    ui->pushButton_Connect->setEnabled(!connected);
    ui->lineEdit_Server->setEnabled(!connected);
    ui->lineEdit_Mysql->setEnabled(!connected);
    ui->pushButton_Disconnect->setEnabled(connected);
    ui->pushButton_Login->setEnabled(connected);
    ui->pushButton_GetImeiList->setEnabled(false);
    ui->pushButton_UpdataImeiData->setEnabled(false);
    ui->tableWidget->setEnabled(connected);

    ui->tableWidget->setRowCount(0);
    ui->label_InProcess_GetImeiList->setText("");
    ui->label_InProcess_UpdataImeiData->setText("");

    return;
}

void MainWindow::on_pushButton_Connect_clicked()
{
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket,SIGNAL(connected()),this,SLOT(slotConnected()));
    connect(tcpSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(slotDataReceived()));

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

    QString strMysql = ui->lineEdit_Mysql->text();
    QRegExp regMysql("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})");
    int iMysqlPos = regMysql.indexIn(strMysql);
    if(iMysqlPos >= 0)
    {
        QString strMysqlHost = regMysql.cap(1);
        int iMysqlPort = regMysql.cap(2).toInt(0, 10);
        qDebug() << "connect to mysql" << strMysqlHost << iMysqlPort;

        data_base.setHostName(strMysqlHost);
        data_base.setPort(iMysqlPort);
    }
    else
    {
        qDebug() << "connect to gDefaultMysql:" << gDefaultMysql;
        ui->lineEdit_Server->setText(gDefaultMysql);

        data_base.setHostName(QString("120.25.157.233"));
        data_base.setPort(3306);
    }

    data_base.setDatabaseName("gps");
    data_base.setUserName("admin");
    data_base.setPassword("xiaoan2016");
    qDebug() << "want to open mysql";
    if(!data_base.open())
    {
        qDebug() << "connect to mysql failed" << data_base.lastError();
    }
    else
    {
        qDebug() << "connect to mysql success";
    }

    return;
}

void MainWindow::on_pushButton_Disconnect_clicked()
{
    tcpSocket->disconnectFromHost();
    data_base.close();

    ui->tableWidget->setRowCount(0); //clear the table
    return;
}

void MainWindow::on_pushButton_Login_clicked()
{
    QByteArray array_header = QByteArray::fromHex("aa6601110000"); //set seq at 0xff for updata imei data loop
    tcpSocket->write(array_header);
}

void MainWindow::on_pushButton_GetImeiList_clicked()
{
    sql_query = QSqlQuery(data_base);
    sql_query.prepare(QString("select imei from object"));
    if(!sql_query.exec())
    {
        qDebug() << sql_query.lastError();
    }
    else
    {
        int iQuerySize = sql_query.size();
        qDebug() << "select imei from object query success" << iQuerySize;
        if(iQuerySize <= 0)
        {
            return;
        }
        else
        {
            ui->tableWidget->setRowCount(0);//clear the table
            ui->label_InProcess_GetImeiList->setText("Geting");
        }

        while(sql_query.next())
        {
            QString imeiString = sql_query.value("imei").toString();

            int rowNum = ui->tableWidget->rowCount();
            ui->tableWidget->setRowCount(rowNum+1);

            ui->tableWidget->setItem(rowNum, 0, new QTableWidgetItem(imeiString));
            ui->tableWidget->setItem(rowNum, 1, new QTableWidgetItem("Details"));
            ui->tableWidget->item(rowNum, 0)->setForeground(Qt::blue);
            ui->tableWidget->item(rowNum, 1)->setForeground(Qt::blue);
        }
        ui->tableWidget->resizeColumnsToContents();
        ui->label_InProcess_GetImeiList->setText("");
    }

    return;
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

    ui->pushButton_Login->setEnabled(false);
    ui->pushButton_GetImeiList->setEnabled(true);
    ui->pushButton_UpdataImeiData->setEnabled(true);
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

        int rowNum = ui->tableWidget->findItems(imei, Qt::MatchExactly).first()->row();

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

        default:
            return -1;
    }
}

int MainWindow::handle_manager_msg(const char *m, size_t msgLen)
{
    const MANAGER_MSG_HEADER *msg = (const MANAGER_MSG_HEADER *)m;

    if(msgLen < MANAGER_MSG_HEADER_LEN)
    {
        qDebug("message length not enough: %zu(at least(%zu))", msgLen, MANAGER_MSG_HEADER_LEN);

        return -1;
    }
    size_t leftLen = msgLen;

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
        msg = (const MANAGER_MSG_HEADER *)(m + msgLen - leftLen);
    }
    return 0;
}

void MainWindow::slotDataReceived()
{
    qDebug() << tcpSocket->bytesAvailable();
    while(tcpSocket->bytesAvailable() > 0)
    {
        QByteArray datagram;
        datagram.resize(tcpSocket->bytesAvailable());
        tcpSocket->read(datagram.data(), datagram.size());

        handle_manager_msg(datagram.data(), datagram.length());
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
        QByteArray ba;
        QString text = QInputDialog::getText(NULL,"AT_CMD","AT Command", QLineEdit::Normal,NULL,&isOK);
        if(isOK)
        {
            //print and read
            //QMessageBox::information(NULL,"Information","Your　comment　is:　<b>"+text+"</b>",QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
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
        pTableMenu->addAction(STR_REBOOT);
        pTableMenu->addAction(STR_UPGRADE);


        connect(pTableMenu, SIGNAL(triggered(QAction *)), this, SLOT(slotTableMenuAction(QAction *)));
        pTableMenu->exec(QCursor::pos());
    }

    if(column == 1) //get event
    {
        EventDialog event(this, gCurrentImeiString, data_base);
        event.exec();
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
