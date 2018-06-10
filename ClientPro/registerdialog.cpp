#include "registerdialog.h"
#include "ui_registerdialog.h"
#include <QMessageBox>
#include <client.h>
#include <logindialog.h>
#include <QValidator>

RegisterDialog::RegisterDialog(QWidget *parent, Client* client) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    this->client = client;
    ui->note->setStyleSheet("color:red");
    QRegExp regx1("[1-9][0-9]+$");
    QRegExp regx2("[1-9a-zA-Z]+");
    QValidator *validator1 = new QRegExpValidator(regx1, ui->QrCodeEdit);
    QValidator *validator2 = new QRegExpValidator(regx2, ui->pwdEdit);
    QValidator *validator3 = new QRegExpValidator(regx2, ui->RepwdEdit);
    ui->QrCodeEdit->setValidator(validator1);
    ui->pwdEdit->setValidator(validator2);
    ui->RepwdEdit->setValidator(validator3);

    connect(client, SIGNAL(s_recvRegis(BYTE)), this, SLOT(on_recvRegis(BYTE)));
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_login_clicked()
{
    this->accept();
}

void RegisterDialog::on_regis_clicked()
{
    string nickname = ui->nicknameEdit->text().toLocal8Bit().data();
    u_int qrCode = ui->QrCodeEdit->text().toUInt();
    string password = ui->pwdEdit->text().toStdString();
    string repassword = ui->RepwdEdit->text().toStdString();
    if (password != repassword) {
        ui->pwdEdit->clear();
        ui->RepwdEdit->clear();
        ui->pwdEdit->setFocus();
        ui->note->setText(QString::fromLocal8Bit("两次输入的密码不相同"));
        return;
    }
    client->regis(qrCode, nickname, password);
    ui->note->setText(QString::fromLocal8Bit("注册请求已发送"));
}

void RegisterDialog::on_recvRegis(BYTE stateCode) {
    if (stateCode == OKAY) {
        ui->note->setText(QString::fromLocal8Bit("注册成功"));
        disconnect(client, SIGNAL(SrecvRegis(BYTE)), this, SLOT(on_recvRegis(BYTE)));
        accept();
    }
    else {
        ui->note->setText(QString::fromLocal8Bit("注册失败"));
        ui->pwdEdit->clear();
        ui->RepwdEdit->clear();
    }
}

