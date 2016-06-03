#include "eventdialog.h"
#include "ui_eventdialog.h"

#include <QDebug>

#define EVENT_NUM_IN_ONE_PAGE 60

static QString gImeiString;
static QSqlDatabase gDatabase;

static int gQuerySize = 0;
static int gPagesTotal = 0;
static int gPagesNow = 0;

EventDialog::EventDialog(QWidget *parent, QString imeiString, QSqlDatabase database) :
    QDialog(parent),
    ui(new Ui::EventDialog)
{
    ui->setupUi(this);
    connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slotHeaderClicked(int)));

    gImeiString = imeiString;
    gDatabase = database;

    setWindowTitle(QString("Event_%1").arg(imeiString));

    sql_query = QSqlQuery(database);
    QString strSelectFromTableLog = QString("select time, event from log where imei='%1' order by time desc").arg(imeiString);

    sql_query.prepare(strSelectFromTableLog);
    if(!sql_query.exec())
    {
        qDebug()<<sql_query.lastError();
    }
    else
    {
        gQuerySize = sql_query.size();
        qDebug()<<"select query success" << gQuerySize;

        if(gQuerySize <= 0)
        {
            qDebug() << "no event" << imeiString;
            ui->label_PageInTotal->setText(QString("/ 0 Pages"));
            ui->lineEdit_PageNum->setText(QString("0"));
            ui->pushButton_Previous->setEnabled(false);
            ui->pushButton_Next->setEnabled(false);

            return;
        }

        gPagesTotal = (gQuerySize + EVENT_NUM_IN_ONE_PAGE - 1) / EVENT_NUM_IN_ONE_PAGE; //进一法
        ui->label_PageInTotal->setText(QString("/ %1 Pages").arg(gPagesTotal));

        gPagesNow = 1;
        displayEventsWithPagesNow();
    }
}

EventDialog::~EventDialog()
{
    delete ui;
    sql_query.clear();

    gQuerySize = 0;
    gPagesTotal = 0;
    gPagesNow = 0;
}

void EventDialog::displayEventsWithPagesNow(void)
{
    if(gPagesNow < 1 || gPagesNow > gPagesTotal)
    {
        qDebug() << "displayEventsWithPagesNow() pages num error, gPagesNow:" << gPagesNow << "gPagesTotal:" << gPagesTotal;
        return;
    }

    int startEventNum = (gPagesNow - 1) * EVENT_NUM_IN_ONE_PAGE;
    bool bSeekRet = sql_query.seek(startEventNum);
    if(bSeekRet == false)
    {
        qDebug() << "sql_query.seek error" << startEventNum;
        return;
    }

    ui->tableWidget->setRowCount(0); //clear the table
    ui->lineEdit_PageNum->setText(QString("%1").arg(gPagesNow));
    if(gPagesNow == 1)
    {
        ui->pushButton_Previous->setEnabled(false);
        ui->pushButton_Next->setEnabled(true);
    }
    else if(gPagesNow == gPagesTotal)
    {
        ui->pushButton_Previous->setEnabled(true);
        ui->pushButton_Next->setEnabled(false);
    }
    else
    {
        ui->pushButton_Previous->setEnabled(true);
        ui->pushButton_Next->setEnabled(true);
    }

    while(sql_query.at() - startEventNum < EVENT_NUM_IN_ONE_PAGE)
    {
        QString timeString = sql_query.value("time").toString();
        QString eventString = sql_query.value("event").toString();

        int rowNum = ui->tableWidget->rowCount();
        ui->tableWidget->setRowCount(rowNum+1);

        ui->tableWidget->setItem(rowNum, 0, new QTableWidgetItem(QString("No.%1").arg(sql_query.at() + 1)));
        ui->tableWidget->setItem(rowNum, 1, new QTableWidgetItem(timeString));
        ui->tableWidget->setItem(rowNum, 2, new QTableWidgetItem(eventString));
        ui->tableWidget->resizeColumnsToContents();

        bool bNextRet = sql_query.next();
        if(bNextRet == false)
        {
            break;
        }
    }
}

void EventDialog::slotHeaderClicked(int column)
{
    qDebug() << "slotHeaderClicked" << column;

    ui->tableWidget->sortByColumn(column);
}

void EventDialog::on_pushButton_Previous_clicked()
{
    qDebug() << "on_pushButton_Previous_clicked";

    gPagesNow--;
    displayEventsWithPagesNow();
}

void EventDialog::on_pushButton_Next_clicked()
{
    qDebug() << "on_pushButton_Next_clicked";

    gPagesNow++;
    displayEventsWithPagesNow();
}

void EventDialog::on_lineEdit_PageNum_returnPressed()
{
    qDebug() << "on_lineEdit_PageNum_returnPressed" << ui->lineEdit_PageNum->text();

    bool bTextToInt;
    int iPageNum = ui->lineEdit_PageNum->text().toInt(&bTextToInt, 10);
    if(bTextToInt == true && iPageNum > 0 && iPageNum <= gPagesTotal)
    {
        gPagesNow = iPageNum;
    }
    else
    {
        qDebug() << "on_lineEdit_PageNum_returnPressed() error";

        ui->lineEdit_PageNum->setText(QString("%1").arg(gPagesNow));
        return;
    }

    displayEventsWithPagesNow();
}
