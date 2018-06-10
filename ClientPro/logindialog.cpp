#include "logindialog.h"
#include "ui_logindialog.h"
#include <iostream>
#include <QMessageBox>
#include <QRegExp>
#include <QRegExpValidator>

LoginDialog::LoginDialog(QWidget *parent, Client* client) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    this->client = client;
    ui->pwdEdit->setEchoMode(QLineEdit::Password);
    //set input valid
    QRegExp regx1("[1-9][0-9]+$");
    QRegExp regx2("[1-9a-zA-Z]+");
    QValidator *validator1 = new QRegExpValidator(regx1, ui->QrCodeEdit);
    QValidator *validator2 = new QRegExpValidator(regx2, ui->pwdEdit);
    ui->QrCodeEdit->setValidator(validator1);
    ui->pwdEdit->setValidator(validator2);
    connect(client, SIGNAL(s_recvLogin(BYTE)), this, SLOT(on_recvLogin(BYTE)));
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_regis_clicked()
{
    RegisterDialog *registerDialog = new RegisterDialog(this, client);
    registerDialog->exec();
}

void LoginDialog::on_login_clicked()
{
    if (ui->QrCodeEdit->text() == "" || ui->pwdEdit->text() == "") {
        QMessageBox::warning(this, NULL, QString::fromLocal8Bit("帐户或者密码不能为空"), QMessageBox::Yes);
        return;
    }
    client->login(ui->QrCodeEdit->text().toUInt(), ui->pwdEdit->text().toStdString());
}

void LoginDialog::on_recvLogin(BYTE stateCode) {
    if (stateCode == OKAY) {
        cout << "Login succ" << endl;
        accept();
    }
    else {
        cout << "Login failed" << endl;
        ui->note->setText(QString::fromLocal8Bit("登录失败，密码或帐户名错误！"));
        ui->pwdEdit->clear();
        ui->pwdEdit->setFocus();
    }
}
