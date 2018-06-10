#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <client.h>

class LoginDialog;

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = 0, Client* client = 0);
    ~RegisterDialog();

public slots:
    void on_recvRegis(BYTE);

private slots:

    void on_login_clicked();

    void on_regis_clicked();

private:
    Ui::RegisterDialog *ui;
    Client* client;
};

#endif // REGISTERDIALOG_H
