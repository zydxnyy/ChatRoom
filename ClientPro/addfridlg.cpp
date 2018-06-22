#include "addfridlg.h"
#include "ui_addfridlg.h"
#include <client.h>

AddFriDlg::AddFriDlg(QWidget *parent, Client* client) :
    QDialog(parent),
    ui(new Ui::AddFriDlg)
{
    ui->setupUi(this);
    this->client = client;
    srchFri = &(client->srchfri);
    connect(client, SIGNAL(s_recvSrchFri(BYTE)), this, SLOT(on_recvSrchFri(BYTE)));
    select_id = 0;
}

AddFriDlg::~AddFriDlg()
{
    delete ui;
}

void AddFriDlg::on_recvSrchFri(BYTE stateCode) {
    if (stateCode == OKAY) {
        ui->fri_list->clear();
        User* u;
        QColor q;
        q.setGreenF(0.3);
        q.setAlphaF(0.5);
        for (int i=0;i<srchFri->size();++i) {
            u = (*srchFri)[i];
            ui->fri_list->addItem(QString::number(u->accountId)+"->"+QString::fromLocal8Bit(u->nickname.c_str()));
            if (u->isOnline) ui->fri_list->item(i)->setBackgroundColor(q);
        }
        select_id = 0;
    }
}

void AddFriDlg::on_search_btn_clicked()
{
    string srch_content = ui->fri_search_content->text().toLocal8Bit().data();
    client->srchFri(srch_content);
}

void AddFriDlg::on_fri_list_currentRowChanged(int currentRow)
{
    if(currentRow<0) return;
    select_id = client->srchfri[currentRow]->accountId;
    cout << "select " << select_id << endl;
}

void AddFriDlg::on_fri_request_clicked()
{
    if (select_id==0) ui->note->setText(QString::fromLocal8Bit("请选择一位用户"));
    else if (!(*srchFri)[ui->fri_list->currentRow()]->isOnline) ui->note->setText(QString::fromLocal8Bit("该用户未上线"));
    else {
        client->addFriRqst(select_id, "我想pick你啊");
        ui->note->setText(QString::fromLocal8Bit("请求已发送"));
        return;

        //不能添加自己和已经是好友的用户为好友
        if (select_id!=client->self.accountId && client->userMap.find(select_id)==client->userMap.end()) {
            client->addFriRqst(select_id, "我想pick你啊");
            ui->note->setText(QString::fromLocal8Bit("请求已发送"));
        }
        else
            ui->note->setText(QString::fromLocal8Bit("该用户已经是您的好友"));
    }
}
