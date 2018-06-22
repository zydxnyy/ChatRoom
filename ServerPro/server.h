#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <algorithm>
#include <windows.h>
#include <MyBuffer.h>
#include <global.h>
#include <winsock2.h>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QDebug>
#include <json/json.h>
#include <set>

class Server
{
    /********************私有成员变量*********************/
    WSADATA wsaData;
    //监听套接字
    SOCKET slisten;
    //监听地址
    sockaddr_in sin;
    UserMap userMap;
    HANDLE rwUserMap;
    QSqlDatabase database;
    //存放匹配请求的
    set<FriRqstPair> FriRqstPairSet;
    /*******************私有处理线程**********************/
    friend DWORD WINAPI listenThreadFunc(LPVOID lpParam);
    friend DWORD WINAPI recvThreadFunc(LPVOID lpParam);
    friend DWORD WINAPI sendThreadFunc(LPVOID lpParam);
    /*******************私有成员函数**********************/
    int fill_in(MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp, const char* sendMsg, BYTE actionCode, BYTE stateCode);
    int fill_in(u_int accountId, const char* sendMsg, BYTE actionCode, BYTE stateCode);

    int recvLogin(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp, u_int& aId);
    int recvRegis(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp);
    int recvChat(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp);
    int recvSrchFri(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp);
    int recvAddFriRqst(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp);
public:
    Server();
    ~Server();
    bool powerOn();
    bool powerOff();
};



struct RsParam {
    Server* server;
    SOCKET* sClient;
    u_int accountId;
    sockaddr_in* clientAddr;
    MyBuffer* sendBuffer;
    HANDLE* rwSendBufferMutex;
    HANDLE* sendBufferSmp;
    bool flag;
    RsParam(Server* server, SOCKET* sClient, sockaddr_in* clientAddr, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp) {
        this->server = server;
        this->sClient = sClient;
        this->clientAddr = clientAddr;
        this->sendBuffer = sendBuffer;
        this->rwSendBufferMutex = rwSendBufferMutex;
        this->sendBufferSmp = sendBufferSmp;
        flag = false;
    }
};

DWORD WINAPI listenThreadFunc(LPVOID lpParam);
DWORD WINAPI recvThreadFunc(LPVOID lpParam);
DWORD WINAPI sendThreadFunc(LPVOID lpParam);

#endif // SERVER_H
