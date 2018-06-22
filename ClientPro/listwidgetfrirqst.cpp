#include "listwidgetfrirqst.h"
#include "ui_listwidgetfrirqst.h"
#include <iostream>
#include <client.h>

listWidgetFriRqst::listWidgetFriRqst(QWidget *parent, Client* client, AddFriRqst* addFriRqst) :
    QWidget(parent),
    ui(new Ui::listWidgetFriRqst)
{
    ui->setupUi(this);
    this->client = client;
    this->addFriRqst = addFriRqst;
    QString qs = QString::fromLocal8Bit("%1(%2)请求添加好友。").arg(QString::fromLocal8Bit(addFriRqst->fromName.c_str())).arg(QString::number(addFriRqst->fromId));
    ui->note->setText(qs);
    ui->comment->setText(QString::fromLocal8Bit(addFriRqst->comment.c_str()));
    std::cout << addFriRqst->fromName+" request to add you with comment:"+addFriRqst->comment << std::endl;
}

listWidgetFriRqst::~listWidgetFriRqst()
{
    delete ui;
}

void listWidgetFriRqst::on_agree_btn_clicked()
{
    client->replyFriRqst(addFriRqst->fromId, AGREE);
    ui->agree_btn->setEnabled(false);
    ui->reject_btn->setEnabled(false);
    ui->label->setText(QString::fromLocal8Bit("你同意了好友请求"));
}

void listWidgetFriRqst::on_reject_btn_clicked()
{
    client->replyFriRqst(addFriRqst->fromId, REJECT);
    ui->agree_btn->setEnabled(false);
    ui->reject_btn->setEnabled(false);
    ui->label->setText(QString::fromLocal8Bit("你拒绝了好友请求"));
}
