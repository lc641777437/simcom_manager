#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QKeyEvent>

#define htonl(l) ((((l >> 24) & 0x000000ff)) | \
                  (((l >>  8) & 0x0000ff00)) | \
                  (((l) & 0x0000ff00) <<  8) | \
                  (((l) & 0x000000ff) << 24))
#define ntohl(l) htonl(l)

#define htons(s) ((((s) >> 8) & 0xff) | \
                  (((s) << 8) & 0xff00))
#define ntohs(s) htons(s)

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_Connect_clicked();
    void on_pushButton_Disconnect_clicked();
    void on_pushButton_Login_clicked();
    void on_pushButton_GetImeiList_clicked();
    void on_pushButton_Get_Local_imeiList_clicked();
    void on_pushButton_UpdataImeiData_clicked();
    void on_tableWidget_cellDoubleClicked(int row, int column);

    void slotConnected();
    void slotDisconnected();
    void slotDataReceived();
    void slotHeaderClicked(int);
    void slotTableMenuAction(QAction *);

    void findInTableWidget(QString string);
    void UpdataImeiDataWithRow(int row);

    void openFile();

private:
    Ui::MainWindow *ui;
    QTcpSocket *tcpSocket;
    QSqlDatabase data_base;
    QSqlQuery sql_query;
    QMenu *pTableMenu;
    QAction *openAction;


    void keyPressEvent(QKeyEvent *event);

    int handle_manager_msg(const char *m, size_t msgLen);
    int handle_one_msg(const void *m);
    int manager_login(const void *msg);
    int manager_imeiData(const void *msg);
    int manager_getLog(const void *msg);
    int manager_get433(const void *msg);
    int manager_getGSM(const void *msg);
    int manager_getGPS(const void *msg);
    int manager_getSetting(const void *msg);
    int manager_getBattery(const void *msg);
    int manager_getAT(const void *msg);
    int manager_setServerRsp(const void *msg);

    void uiShowConnectionStatus(bool connected);
    void uiShowImeiData(const char *imei, char online_offline, int version, int timestamp, float longitude, float latitude, char speed, short course);
};

#endif // MAINWINDOW_H
