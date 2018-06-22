#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <stdio.h>
#include <map>
#include<WINSOCK2.H>
#include<windows.h>
#include <string>
#include<cstring>
#include <Ws2tcpip.h>
#include <json/json.h>
#include <MyBuffer.h>
#include <global.h>
#include <CUtil.h>
#include <vector>
using namespace std;

class Client : public QObject
{
    Q_OBJECT
public:
    friend class LoginDialog;
    friend class MainWindow;
    friend class RegisterDialog;
    friend class AddFriDlg;
    friend class listWidgetFriRqst;
    friend class Notification;
    explicit Client(QObject *parent = nullptr);
    ~Client();
    bool powerOn();
    void powerOff();
    int login(u_int accountId, const string& password);
    int regis(u_int accountId, const string& nickname, const string& password);
    int chat(u_int friend_accountId, const string& chatMsg);
    int srchFri(const string& srchContent);
    int addFriRqst(u_int accountId, const string& comment);
    int replyFriRqst(u_int accountId, BYTE flag);
signals:
    void s_recvLogin(BYTE);
    void s_recvRegis(BYTE);
    void s_recvChat(u_int);
    void s_recvNotifyLogin(u_int);
    void s_recvLogout(u_int);
    void s_recvSrchFri(BYTE);
    void s_recvAddFriRqst(BYTE);

public slots:

private:
    /**************˽�г�Ա����***************/
    //�ͻ���ͨ��socket
    SOCKET sclient;
    // Public Declarations
    //�Ժ���map��д������
    HANDLE wUserMap;
    //�����б�
    UserMap userMap;
    //WSA����
    WSADATA data;
    //�������˵�ַ
    sockaddr_in serAddr;
    //������Ϣ������
    MyBuffer sendBuffer;
    //������Ϣ������
    MyBuffer recvBuffer;
    //��д���ͻ�����������
    HANDLE RWSendBufferMutex;
    //��д���ܻ�����������
    HANDLE RWRecvBufferMutex;
    //���ͻ������¼�
    HANDLE sendBufferSmp;
    //�����߳̾��
    HANDLE HDSend;
    //�����߳̾��
    HANDLE HDRecv;
    //�����߳̾��
    HANDLE HDHeartbeat;
    //�û��Լ�
    User self;
    //�洢���������û�
    vector<User*> srchfri;
    //�洢��Ӻ��ѵ�֪ͨ
    vector<AddFriRqst> vAddFriRqst;
    /************˽�г�Ա����**************/
    /*�����̣߳����������������
     *
     */
    friend DWORD WINAPI sendThreadFunc(LPVOID lpParam);
    friend DWORD WINAPI recvThreadFunc(LPVOID lpParam);
    friend DWORD WINAPI heartbeatThreadFunc(LPVOID lpParam);

    /*����������Ϣ*/
    int recvLogin();
    int recvRegister();
    int recvChat();
    int recvNotifiyLogin();
    int recvLogout();
    int recvSrchFri();
    int recvFriRqst();
    int recvAddFriRqst();

    int handleEvent();

    /*��䷢�ͻ���������*/
    int fill_in(const char* sendMsg, const BYTE& actionCode, const BYTE& stateCode);

    bool conn();
};


DWORD WINAPI sendThreadFunc(LPVOID lpParam);
DWORD WINAPI recvThreadFunc(LPVOID lpParam);
DWORD WINAPI heartbeatThreadFunc(LPVOID lpParam);

#endif // CLIENT_H
