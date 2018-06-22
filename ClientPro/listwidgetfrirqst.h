#ifndef LISTWIDGETFRIRQST_H
#define LISTWIDGETFRIRQST_H

#include <QWidget>
#include <global.h>

namespace Ui {
class listWidgetFriRqst;
}

class listWidgetFriRqst : public QWidget
{
    Q_OBJECT

public:
    explicit listWidgetFriRqst(QWidget *parent = 0, Client* client = 0, AddFriRqst* addFriRqst = 0);
    ~listWidgetFriRqst();
private slots:
    void on_agree_btn_clicked();

    void on_reject_btn_clicked();

private:
    Ui::listWidgetFriRqst *ui;
    AddFriRqst* addFriRqst;
    Client* client;
};

#endif // LISTWIDGETFRIRQST_H
