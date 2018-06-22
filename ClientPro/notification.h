#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QDialog>
#include <global.h>

namespace Ui {
class Notification;
}

class Notification : public QDialog
{
    Q_OBJECT

public:
    explicit Notification(QWidget *parent = 0, Client* client = 0);
    ~Notification();

public slots:
    void on_recvFriRqst(BYTE);

private:
    Ui::Notification *ui;
    Client* client;
};

#endif // NOTIFICATION_H
