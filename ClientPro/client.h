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
using namespace std;

class Client : public QObject
{
    Q_OBJECT
public:
    friend class LoginDialog;
    friend class MainWindow;
    friend class RegisterDialog;
    friend class AddFriDlg;
    explicit Client(QObject *parent = nullptr);
    ~Client();
    bool powerOn();
    void powerOff();
    int login(u_int accountId, const string& password);
    int regis(u_int accountId, const string& nickname, const string& password);
    int chat(u_int friend_accountId, const string& chatMsg);
    int srchFri(const string& srchContent);
signals:
    void s_recvLogin(BYTE);
    void s_recvRegis(BYTE);
    void s_recvChat(u_int);
    void s_recvNotifyLogin(u_int);
    void s_recvLogout(u_int);
    void s_recvSrchFri(BYTE);

public slots:

private:
    /**************私有成员变量***************/
    //客户端通信socket
    SOCKET sclient;
    // Public Declarations
    //对好友map的写互斥量
    HANDLE wUserMap;
    //好友列表
    UserMap userMap;
    //WSA数据
    WSADATA data;
    //服务器端地址
    sockaddr_in serAddr;
    //发送信息缓冲区
    MyBuffer sendBuffer;
    //接收信息缓冲区
    MyBuffer recvBuffer;
    //读写发送缓冲区互斥量
    HANDLE RWSendBufferMutex;
    //读写接受缓冲区互斥量
    HANDLE RWRecvBufferMutex;
    //发送缓冲区事件
    HANDLE sendBufferSmp;
    //发送线程句柄
    HANDLE HDSend;
    //接收线程句柄
    HANDLE HDRecv;
    //心跳线程句柄
    HANDLE HDHeartbeat;
    //用户自己
    User self;
    //存储搜索来的用户
    vector<User*> srchfri;
    /************私有成员函数**************/
    /*三个线程，心跳，发送与接收
     *
     */
    friend DWORD WINAPI sendThreadFunc(LPVOID lpParam);
    friend DWORD WINAPI recvThreadFunc(LPVOID lpParam);
    friend DWORD WINAPI heartbeatThreadFunc(LPVOID lpParam);

    /*解析接收信息*/
    int recvLogin();
    int recvRegister();
    int recvChat();
    int recvNotifiyLogin();
    int recvLogout();
    int recvSrchFri();

    int handleEvent();

    /*填充发送缓冲区函数*/
    int fill_in(const char* sendMsg, const BYTE& actionCode, const BYTE& stateCode);
};


DWORD WINAPI sendThreadFunc(LPVOID lpParam);
DWORD WINAPI recvThreadFunc(LPVOID lpParam);
DWORD WINAPI heartbeatThreadFunc(LPVOID lpParam);

#endif // CLIENT_H
