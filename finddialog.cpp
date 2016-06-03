#include "finddialog.h"
#include "ui_finddialog.h"

#include <QDebug>

FindDialog::FindDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindDialog)
{
    ui->setupUi(this);

    setWindowTitle(QString("Find in Table"));

    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(on_pushButton_ok_clicked()));
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::on_pushButton_ok_clicked()
{
    qDebug() << "on_pushButton_ok_clicked" << ui->lineEdit->text();

    if(ui->lineEdit->text().isEmpty())
    {
        qDebug() << "empty line edit";
        return;
    }
    else
    {
        findString(ui->lineEdit->text());

        this->close();
    }
}

void FindDialog::on_pushButton_cancel_clicked()
{
    this->close();
}

