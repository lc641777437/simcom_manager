#include "eventdialog.h"
#include "ui_eventdialog.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

#define EVENT_NUM_IN_ONE_PAGE 60

static QString gImeiString;
static QSqlDatabase gDatabase;

static int gQuerySize = 0;
static int gPagesTotal = 0;
static int gPagesNow = 0;

EventDialog::EventDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventDialog)
{
    ui->setupUi(this);
    connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slotHeaderClicked(int)));

    setWindowTitle(QString("EventDialog"));
}

EventDialog::~EventDialog()
{
    delete ui;
}

void EventDialog::event_Display(void)
{
}

void EventDialog::slotHeaderClicked(int column)
{
    qDebug() << "slotHeaderClicked" << column;

    ui->tableWidget->sortByColumn(column);
}

void EventDialog::on_pushButton_Previous_clicked()
{
    qDebug() << "on_pushButton_Previous_clicked";
}

void EventDialog::on_pushButton_Next_clicked()
{
    qDebug() << "on_pushButton_Next_clicked";
}

void EventDialog::on_lineEdit_PageNum_returnPressed()
{
    qDebug() << "on_lineEdit_PageNum_returnPressed" << ui->lineEdit_PageNum->text();
}

void EventDialog::get_daily2Eventdialog(QString data)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data.toLocal8Bit().data());
    if( jsonDocument.isNull() )
    {
        qDebug()<< "please check the string "<< data.toLocal8Bit().data();
    }

    QJsonObject jsonObject = jsonDocument.object();
    int rowNum = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(rowNum+1);
    ui->tableWidget->setItem(rowNum, 0, new QTableWidgetItem(jsonObject.take("time").toString()));
    ui->tableWidget->setItem(rowNum, 1, new QTableWidgetItem(jsonObject.take("event").toString()));
    ui->tableWidget->resizeColumnsToContents();
}
void EventDialog::get_dtart2Eventdialog(void)
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
}
