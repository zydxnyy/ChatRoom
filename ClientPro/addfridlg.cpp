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
}

AddFriDlg::~AddFriDlg()
{
    delete ui;
}

void AddFriDlg::on_recvSrchFri(BYTE stateCode) {
    if (stateCode == OKAY) {
        ui->fri_list->clear();
        User* u;
        for (int i=0;i<srchFri->size();++i) {
            u = (*srchFri)[i];
            ui->fri_list->addItem(QString::number(u->accountId)+"->"+QString::fromLocal8Bit(u->nickname.c_str()));
        }
    }
}

void AddFriDlg::on_search_btn_clicked()
{
    string srch_content = ui->fri_search_content->text().toLocal8Bit().data();
    client->srchFri(srch_content);
}
