#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace Ui {
class EventDialog;
}
class MainWindow;
class EventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EventDialog(QWidget *parent);
    ~EventDialog();
    MainWindow *pMainWindow;

private slots:
    void slotHeaderClicked(int);

    void on_pushButton_Previous_clicked();
    void on_pushButton_Next_clicked();
    void on_lineEdit_PageNum_returnPressed();

private:
    Ui::EventDialog *ui;

    void event_Display(void);

public slots:
    void get_daily2Eventdialog(QString);
    void get_dtart2Eventdialog(void);
};

#endif // EVENTDIALOG_H
