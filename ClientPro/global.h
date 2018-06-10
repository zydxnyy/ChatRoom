#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <string.h>
#include <sstream>
#include <time.h>

#define MAX_LENGTH 4096
#define MAX_NAME_LENGTH 48
#define LISTEN_PORT 9876
#define HOST_ADDR "127.0.0.1"
#define UserMap std::map<u_int,User*>
#define memccpy _memccpy

typedef unsigned char byte, BYTE;
typedef unsigned int u_int;

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

//好友的信息
struct User {
    u_int accountId;
    std::string nickname;
    bool isOnline;
    int row;
    std::stringstream history;
    User() {}
    User(u_int aId, const std::string& n, bool isOnline) {
        accountId = aId;
        nickname = n;
        this->isOnline = isOnline;
    }
    void set(u_int aId, const std::string& n, bool isOnline) {
        accountId = aId;
        nickname = n;
        this->isOnline = isOnline;
    }
    void setRow(int row) {
        this->row = row;
    }
    void setOnline() {
        isOnline = true;
    }
    void setOffline() {
        isOnline = false;
    }
    void print() {
        printf("<%s : %d : online?->%d>\n", nickname.c_str(), accountId, isOnline);
    }
};

class Client;
class LoginDialog;
class MainWindow;
class RegisterDialog;
class AddFriDlg;

#endif // GLOBAL_H
