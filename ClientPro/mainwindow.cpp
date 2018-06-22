#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <client.h>
#include <QStringList>
#include <QStringListModel>
#include <QKeyEvent>
#include <addfridlg.h>
#include <listwidgetfrirqst.h>

void deleteAtI(std::vector<u_int>& v, int pos) {
    std::vector<u_int>::iterator itr=v.begin();
    itr += pos;
    v.erase(itr);
}

int findRow(std::vector<u_int>& v, int accountId) {
    for (int i=0;i<v.size();++i) {
        if (v[i]==accountId) return i;
    }
    return -1;
}

MainWindow::MainWindow(QWidget *parent, Client* client) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->client = client;
    userMap = &(client->userMap);
    connect(client, SIGNAL(s_recvChat(u_int)), this, SLOT(on_recvChat(u_int)));
    connect(client, SIGNAL(s_recvLogout(u_int)), this, SLOT(on_recvLogout(u_int)));
    connect(client, SIGNAL(s_recvNotifyLogin(u_int)), this, SLOT(on_recvNotifyLogin(u_int)));
    ui->chatBoard->setReadOnly(true);
    ui->sendMsgEdit->installEventFilter(this);

    notificationDlg = new Notification(this, client);
    afd = new AddFriDlg(this, client);
    yColor.setAlphaF(0.3);
    yColor.setRedF(1);

    select_id = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init() {
    UserMap::iterator itr;
    u_int accountId; char* nickName; bool isOnline;
    int z = 0;
    for (itr=userMap->begin();itr!=userMap->end();++itr) {
        accountId = itr->second->accountId;
        nickName = (char*)itr->second->nickname.c_str();
        isOnline = itr->second->isOnline;
//        ui->friendList->addAction(new QAction(QString::number(accountId)+QString::fromLocal8Bit(nickName)));
        v.push_back(accountId);
        ui->friendList->addItem(QString::number(accountId)+"->"+QString::fromLocal8Bit(nickName));
        if (isOnline) ui->friendList->item(z)->setBackgroundColor(yColor);
        ++z;
    }
    ui->accountId->setText(QString::number(client->self.accountId));
    ui->nickname->setText(QString::fromLocal8Bit(client->self.nickname.c_str()));
}

void MainWindow::on_recvNotifyLogin(u_int accountId) {
    User* user = client->userMap[accountId];
    if (user==NULL) return;
    int row = findRow(v, accountId);
    //ÒÑ¾­´æÔÚ
    if (row!=-1) {
        QColor q;
        q.setAlphaF(0.3);
        q.setGreenF(0.7);
        ui->friendList->item(row)->setBackgroundColor(q);
    }
    else {
        v.push_back(accountId);
        ui->friendList->addItem(QString::number(user->accountId)+"->"+QString::fromLocal8Bit(user->nickname.c_str()));
    }
}

void MainWindow::on_friendList_currentRowChanged(int currentRow)
{
    if (currentRow<0) return;
    cout << currentRow << " be change aId = " << v[currentRow] << endl;
    select_id = v[currentRow];
    ui->chatBoard->setText(QString::fromLocal8Bit((*userMap)[select_id]->history.str().c_str()));
    ui->sendMsgEdit->setFocus();
    ui->fri_accountId->setText(QString::number(select_id));
    ui->fri_nickname->setText(QString::fromLocal8Bit((*userMap)[select_id]->nickname.c_str()));

    QTextCursor cursor = ui->chatBoard->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chatBoard->setTextCursor(cursor);

}

void MainWindow::on_send_clicked()
{
    if (select_id == 0) return;
    string chatMsg = ui->sendMsgEdit->toPlainText().toLocal8Bit().data();
    if (chatMsg.empty()) return;
    cout << "Ready to send" << endl;
    client->chat(select_id, chatMsg);
    ui->chatBoard->setText(QString::fromLocal8Bit((*userMap)[select_id]->history.str().c_str()));
    ui->sendMsgEdit->clear();
    ui->sendMsgEdit->setFocus();

    QTextCursor cursor = ui->chatBoard->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chatBoard->setTextCursor(cursor);
}

void MainWindow::on_recvChat(u_int accountId) {

    if (select_id == accountId) {
        ui->chatBoard->setText(QString::fromLocal8Bit((*userMap)[select_id]->history.str().c_str()));
        QTextCursor cursor = ui->chatBoard->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->chatBoard->setTextCursor(cursor);
    }
    int row = findRow(v, accountId);
    yColor.setAlphaF(0.3);
    ui->friendList->item(row)->setBackgroundColor(yColor);

}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    Q_ASSERT(obj == ui->sendMsgEdit);
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *event = static_cast<QKeyEvent*>(e);
        if (event->key() == Qt::Key_Return && (event->modifiers() & Qt::ControlModifier))
        {
            cout << "KEY" << endl;
            ui->send->click();
            return true;
        }
    }
    return false;
}

void MainWindow::on_recvLogout(u_int logoutId) {
    int row = findRow(v, logoutId);
    yColor.setAlphaF(0);
    ui->friendList->item(row)->setBackgroundColor(yColor);
}

void MainWindow::on_friendList_itemSelectionChanged()
{
}

void MainWindow::on_friendList_itemClicked(QListWidgetItem *item)
{
    yColor.setAlphaF(0);
    item->setBackgroundColor(yColor);
}

void MainWindow::on_add_fri_triggered()
{
    afd->exec();
}

void MainWindow::on_action_triggered()
{
    notificationDlg->exec();
}
