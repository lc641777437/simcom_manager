#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace Ui {
class EventDialog;
}

class EventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EventDialog(QWidget *parent, QString imeiString, QSqlDatabase database);
    ~EventDialog();

private slots:
    void slotHeaderClicked(int);

    void on_pushButton_Previous_clicked();
    void on_pushButton_Next_clicked();
    void on_lineEdit_PageNum_returnPressed();

private:
    Ui::EventDialog *ui;
    QSqlQuery sql_query;

    void displayEventsWithPagesNow(void);
};

#endif // EVENTDIALOG_H
