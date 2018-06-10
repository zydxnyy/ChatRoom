#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <global.h>
#include <vector>
#include <QColor>
#include <QListWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, Client* client = 0);
    ~MainWindow();
    void init();
private:
    Ui::MainWindow *ui;
    Client* client;
    UserMap* userMap;
    //�����б�λ����id��ӳ��
    std::vector<u_int> v;
    //ѡ�е�id
    u_int select_id;
    QColor yColor;
public slots:
    void on_recvNotifyLogin(u_int);
    void on_recvChat(u_int);
    void on_recvLogout(u_int);
private slots:
    void on_friendList_currentRowChanged(int currentRow);
    void on_send_clicked();
    void on_friendList_itemSelectionChanged();
    void on_friendList_itemClicked(QListWidgetItem *item);

    void on_add_fri_triggered();

protected:
    bool eventFilter(QObject *target, QEvent *event);//�¼�������
};

#endif // MAINWINDOW_H
