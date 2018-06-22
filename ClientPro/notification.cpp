#include "notification.h"
#include "ui_notification.h"
#include <listwidgetfrirqst.h>
#include <qdebug.h>
#include <client.h>

Notification::Notification(QWidget *parent, Client* client) :
    QDialog(parent),
    ui(new Ui::Notification)
{
    ui->setupUi(this);
    this->client = client;
    connect(client, SIGNAL(s_recvAddFriRqst(BYTE)), this, SLOT(on_recvFriRqst(BYTE)));
}

Notification::~Notification()
{
    delete ui;
}


void Notification::on_recvFriRqst(BYTE stateCode) {
    QListWidgetItem *nitem = new QListWidgetItem(ui->notification);
    nitem->setSizeHint(QSize(400, 150));
    ui->notification->addItem(nitem);
    listWidgetFriRqst* newItem = new listWidgetFriRqst(this, client, &client->vAddFriRqst.back());
    ui->notification->setItemWidget(nitem, newItem);
    cout << "notification add note" << endl;
}
