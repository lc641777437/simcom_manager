#include "eventdialog.h"
#include "ui_eventdialog.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

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
    if(jsonObject.contains(QString("log")))
    {
        int rowNum = 0;
        QJsonValue array_value = jsonObject.take(QString("log"));
        QJsonArray array = array_value.toArray();
        for(int i = 0;i < array.size();i++)
        {
            QJsonValue obj_value = array.at(i);
            if(obj_value.isObject())
            {
                QJsonObject obj = obj_value.toObject();
                ui->tableWidget->setRowCount(rowNum + 1);
                ui->tableWidget->setItem(rowNum, 0, new QTableWidgetItem(obj.take("time").toString()));
                ui->tableWidget->setItem(rowNum, 1, new QTableWidgetItem(obj.take("event").toString()));
                rowNum++;
            }
            ui->tableWidget->resizeColumnsToContents();
        }
    }
}
void EventDialog::get_dtart2Eventdialog(void)
{
    ui->tableWidget->setRowCount(0);
}
