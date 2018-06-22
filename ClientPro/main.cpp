#include <QApplication>
#include <logindialog.h>
#include <registerdialog.h>
#include <mainwindow.h>
#include <client.h>
#include <QTextCodec>
#include <QTranslator>
#include <QMessageBox>
#include <QMetaType>
#include <QtSql/QSqlDatabase>
#include <QtSql/qsqlquery.h>
int main(int argc, char *argv[])
{
    stringstream s;
//    cout << ctime((const time_t*)time(NULL)) << endl;
    qRegisterMetaType<BYTE>("BYTE");
    qRegisterMetaType<u_int>("u_int");
    QApplication a(argc, argv);

    Client client;
    if(!client.powerOn()) {
        return -1;
    }
    MainWindow m(0, &client);
    LoginDialog loginDialog(0, &client);
    if (loginDialog.exec()==QDialog::Accepted) {
        m.init();
        m.show();
    }
    return a.exec();
}
