#ifndef GLOBAL_H
#define GLOBAL_H

#include <server.h>

#define MAX_LENGTH 4096
#define MAX_NAME_LENGTH 48
#define LISTEN_PORT 9876
#define HOST_ADDR "127.0.0.1"
#define UserMap std::map<u_int,User*>
#define memccpy _memccpy

typedef unsigned char byte, BYTE;

enum {
    HEARTBEAT,
    LOGIN,
    REGISTER,
    CHAT,
    GROUPCHAT,
    ADD_FRIEND_REQUEST,
    ACK_FRIEND_REQUEST,
    NOTIFY_LOGIN,
    LOGOUT,
    SEARCH_ACCOUNT,
};

//状态码
enum {
    SEND,   	//发送状态
    OKAY,   	//回传OK状态
    ERR,    	//出错
    PARSE_FAIL,	//解析失败
    NOT_AGREE,  //对方拒绝
};

//在线用户的信息
struct User {
    u_int accountId;
    string nickname;
    MyBuffer* recvBuffer;
    MyBuffer* sendBuffer;
    HANDLE* rwSendBufferMutex;
    HANDLE* sendBufferSmp;
    User(u_int aId, const string& nickname, MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp) {
        accountId = aId;
        this->nickname = nickname;
        this->recvBuffer = recvBuffer;
        this->sendBuffer = sendBuffer;
        this->rwSendBufferMutex = rwSendBufferMutex;
        this->sendBufferSmp = sendBufferSmp;

    }
};

#endif // GLOBAL_H
