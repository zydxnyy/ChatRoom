#ifndef ADDFRIDLG_H
#define ADDFRIDLG_H

#include <QDialog>
#include <global.h>
#include <vector>

namespace Ui {
class AddFriDlg;
}

class AddFriDlg : public QDialog
{
    Q_OBJECT

public:
    explicit AddFriDlg(QWidget *parent = 0, Client* client = 0);
    ~AddFriDlg();
public slots:
    void on_recvSrchFri(BYTE);
private slots:
    void on_search_btn_clicked();

    void on_fri_list_currentRowChanged(int currentRow);

    void on_fri_request_clicked();

private:
    Ui::AddFriDlg *ui;
    Client* client;
    std::vector<User*>* srchFri;
    u_int select_id;
};

#endif // ADDFRIDLG_H
