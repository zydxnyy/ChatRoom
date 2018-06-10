#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QObject>
#include <registerdialog.h>
#include <client.h>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0, Client* client = 0);
    ~LoginDialog();

public slots:
    void on_recvLogin(BYTE);

private slots:

    void on_regis_clicked();

    void on_login_clicked();

private:
    Ui::LoginDialog *ui;
    Client* client;
};

#endif // LOGINDIALOG_H
