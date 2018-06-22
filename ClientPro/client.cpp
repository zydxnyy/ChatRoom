#include "client.h"

string getTime()
{
    time_t timep;
    time (&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
    return tmp;
}
std::string encript(const std::string& m) {
    return QCryptographicHash::hash(QByteArray::fromStdString(m), QCryptographicHash::Md5).toStdString();
}


Client::Client(QObject *parent) : QObject(parent), vAddFriRqst(1024)
{

}

Client::~Client() {
    UserMap::iterator itr;
    for (itr=userMap.begin();itr!=userMap.end();++itr) {
        delete itr->second;
    }
    TerminateThread(HDSend, 0);
    TerminateThread(HDHeartbeat, 0);
    TerminateThread(HDRecv, 0);
}

bool Client::powerOn() {
    //socket版本号
    WORD sockVersion = MAKEWORD(2,   2);

    //获取WSA数据
    if(WSAStartup(sockVersion, &data)!=0) {
        cout << "Errro start WSA!!!" << endl;
        return false;
    }

//    //获取客户端连接socket
    sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //获取失败
    if(sclient == INVALID_SOCKET) {
        cout << "invalid socket!" << endl;
        return false;
    }
    //指定服务器地址
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(LISTEN_PORT);
    serAddr.sin_addr.S_un.S_addr = inet_addr(HOST_ADDR);
//	inet_pton(AF_INET, HOST_ADDR, &serAddr.sin_addr.S_un.S_addr);

    if (::connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
        cout << "Connect error!!!\n";
        return false;
    }
    int nNetTimeout = 5000;
    //设置发送与接收的超时时间为5s
    setsockopt(sclient, SOL_SOCKET, SO_RCVTIMEO, (char*)&nNetTimeout, sizeof(int));
    setsockopt(sclient, SOL_SOCKET, SO_SNDTIMEO, (char*)&nNetTimeout, sizeof(int));

    //创建读写缓冲区互斥量
    RWSendBufferMutex = CreateMutex(NULL, false, NULL);
    RWRecvBufferMutex = CreateMutex(NULL, false, NULL);
    wUserMap = CreateMutex(NULL, false, NULL);
    sendBufferSmp = CreateSemaphore(NULL, 0, 10, NULL);
    HDSend = CreateThread(NULL, 0, sendThreadFunc, this, 0, NULL);
    HDRecv = CreateThread(NULL, 0, recvThreadFunc, this, 0, NULL);
    HDHeartbeat = CreateThread(NULL, 0, heartbeatThreadFunc, this, 0, NULL);

    //客户端开启成功
    return true;
}

DWORD WINAPI heartbeatThreadFunc(LPVOID lpParam) {
    Client* client = (Client*)lpParam;

    int ret = 0;
    char sendData[6];
    sendData[4] = HEARTBEAT; sendData[5] = SEND;
    *(u_int*)sendData = 6;
    while (true) {
        ret = send(client->sclient, sendData, 6, 0);
        //短线重连
        if (ret == -1) {
            if (!client->conn()) break;
        }
        Sleep(3000);
    }

    return 0;
}

bool Client::conn() {
    u_int deadCnt = 0;
    //获取客户端连接socket
    closesocket(sclient);
    SOCKET sclientNew = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sclient = sclientNew;
    while (::connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
        cout << "trying to reconnect" << endl;
        if (deadCnt++ > 10) {
            closesocket(sclient);
            cout << "Connect error" << endl;
            return false;
        }
        Sleep(3000);
    }
    return true;
}

DWORD WINAPI recvThreadFunc(LPVOID lpParam) {
    Client* client = (Client*) lpParam;
    SOCKET& sclient = client->sclient;
    UserMap& userMap = client->userMap;
    MyBuffer& recvBuffer = client->recvBuffer;
    HANDLE& RWRecvBufferMutex = client->RWRecvBufferMutex;
    int ret = 0;
    char* recvData = NULL;
    while (true) {
        recvData = recvBuffer.getBufferEnd();
        ret = recv(sclient, recvData, recvBuffer.getRest(), 0);
        if (ret == -1) {
            Sleep(500);
            continue;
        }
        recvBuffer.setEnd(ret);
        cout << "ret = " << ret << endl;
        while (recvBuffer.getBufferEnd() != recvBuffer.getBufferHead()) client->handleEvent();
    }

    return 0;
}

DWORD WINAPI sendThreadFunc(LPVOID lpParam) {
    Client* client= (Client*)lpParam;
    SOCKET& sclient = client->sclient;
    UserMap& userMap = client->userMap;
    MyBuffer& sendBuffer = client->sendBuffer;
    HANDLE& RWSendBufferMutex = client->RWSendBufferMutex;
    HANDLE& sendBufferSmp = client->sendBufferSmp;

    int ret = 0;//发送返回值
    char* sendData = NULL;//缓冲区指针
    u_int length = 0;//发送报文长度
    while (true) {
        WaitForSingleObject(sendBufferSmp, INFINITE);
        //申请发送缓冲区
        WaitForSingleObject(RWSendBufferMutex, INFINITE);
        sendData = sendBuffer.getBufferHead();
        length = *(u_int*)sendData;
        ret = send(sclient, sendData, length, 0);
        printf("socket = %d send ret = %d sendL = %u\n", sclient, ret, length);
        sendBuffer.setHead(length);
        if (ret == -1) {
            printf("send error!!!\n");
        }
        //释放互斥量
        ReleaseMutex(RWSendBufferMutex);
    }
    return 0;
}

int Client::handleEvent(){
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取动作码
    byte actionCode = recvData[4];
    printf("actionCode=%02X,stateCode=%02X\n", actionCode, 0);
    switch (actionCode) {
    case HEARTBEAT: {
        break;
    }
    case LOGIN: {
        recvLogin();
        break;
    }
    case CHAT: {
        recvChat();
        break;
    }
    case REGISTER: {
        recvRegister();
        break;
    }
    case NOTIFY_LOGIN:{
        recvNotifiyLogin();
        break;
    }
    case LOGOUT:{
        recvLogout();
        break;
    }
    case SEARCH_ACCOUNT:{
        recvSrchFri();
        break;
    }
    case ADD_FRIEND_REQUEST: {
        recvAddFriRqst();
        break;
    }
    }
    recvBuffer.setHead(recvDataLength);
}

int Client::fill_in(const char* sendMsg, const BYTE& actionCode, const BYTE& stateCode) {
    WaitForSingleObject(RWSendBufferMutex, INFINITE);

    BYTE* sendData = (BYTE*)sendBuffer.getBufferEnd();
    u_int sendMsgLength = 0;
    if (sendMsg!=NULL) sendMsgLength = strlen(sendMsg);
    *(u_int*)sendData = sendMsgLength+6;
    *(sendData+4) = actionCode;
    *(sendData+5) = stateCode;
    if (sendMsg!=NULL) memccpy(sendData+6, sendMsg, 0, sendMsgLength+6);
    sendBuffer.setEnd(sendMsgLength+6);

    ReleaseMutex(RWSendBufferMutex);
    ReleaseSemaphore(sendBufferSmp, 1, NULL);

    return 0;
}

int Client::login(u_int accountId, const string& password) {
    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["accountId"] = accountId;
    root["password"] = password;
    std::string s = Json::writeString(wbuilder, root);
    fill_in(s.c_str(), LOGIN, SEND);
    return 0;
}

int Client::regis(u_int accountId, const string& nickname, const string& password) {
    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["accountId"] = accountId;
    root["nickname"] = string_To_UTF8(nickname);
    root["password"] = password;
    /*可重用代码，产生整个报文*/
    std::string s = Json::writeString(wbuilder, root);
    fill_in(s.c_str(), REGISTER, SEND);
    return 0;
}

int Client::chat(u_int friend_accountId, const string& chatMsg) {
    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["friendAccountId"] = friend_accountId;
    root["accountId"] = self.accountId;
    root["chatMessage"] = string_To_UTF8(chatMsg);
    std::string s = Json::writeString(wbuilder, root);
    fill_in(s.c_str(), CHAT, SEND);
    UserMap::iterator itr=userMap.find(friend_accountId);
    if (itr!=userMap.end())
        itr->second->history << self.nickname << " (" << self.accountId << ") " << getTime() << ":\n" << chatMsg << "\n\n";
    else
        cout << "chat error" << endl;
    return 0;
}

int Client::srchFri(const string& srch_content) {
    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["search_content"] = string_To_UTF8(srch_content);
    std::string s = Json::writeString(wbuilder, root);
    fill_in(s.c_str(), SEARCH_ACCOUNT, SEND);
    return 0;
}

int Client::addFriRqst(u_int accountId, const string &comment) {
    if (userMap.find(accountId)!=userMap.end()) return -1;

    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["addId"] = accountId;
    root["comment"] = string_To_UTF8(comment);
    root["fromId"] = self.accountId;
    root["fromName"] = string_To_UTF8(self.nickname);
    /*可重用代码，产生整个报文*/
    std::string s = Json::writeString(wbuilder, root);
    fill_in(s.c_str(), ADD_FRIEND_REQUEST, SEND);
    return 0;
}

int Client::replyFriRqst(u_int accountId, BYTE flag) {
    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["addId"] = self.accountId;
    root["comment"] = string_To_UTF8("我答应你了");
    root["fromId"] = accountId;
    root["addName"] = string_To_UTF8(self.nickname);
    /*可重用代码，产生整个报文*/
    std::string s = Json::writeString(wbuilder, root);
    fill_in(s.c_str(), ADD_FRIEND_REQUEST, flag);
    return 0;
}

int Client::recvLogin() {
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取状态码
    BYTE stateCode = recvData[5];
    if (stateCode == OKAY) {
        Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
        Json::Value root, friends;
        JSONCPP_STRING errs;
        bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
        //解析成功
        if (ok&&errs.size() == 0)
        {
            friends = root["friend"];
            u_int accountId; bool isOnline; std::string nickname;
            for (u_int i = 0; i < friends.size(); ++i) {
                accountId = friends[i]["accountId"].asUInt();
                isOnline = friends[i]["isOnline"].asBool();
                nickname = UTF8_To_string(friends[i]["nickname"].asString());
                userMap[accountId] = new User(accountId, nickname, isOnline);
            }
            self.set(root["accountId"].asUInt(), UTF8_To_string(root["nickname"].asString()), true);
        }
        //解析失败
        else {
            cout << errs << endl;
        }
        delete reader;

    }
    else {
        cout << "Login failed" << endl;
    }
    emit s_recvLogin(stateCode);
}

int Client::recvRegister() {
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取状态码
    byte stateCode = recvData[5];

    if (stateCode == OKAY) {
        cout << "Regis successfully" << endl;
    }
    else {
        cout << "Regis failed" << endl;
    }

    emit s_recvRegis(stateCode);
}

int Client::recvChat() {
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取状态码
    byte stateCode = recvData[5];
    Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
    Json::Value root;
    JSONCPP_STRING errs;
    bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
    //解析成功
    if (ok&&errs.size() == 0)
    {
        u_int from = root["fromAccountId"].asUInt();
        string chatMsg = UTF8_To_string(root["chatMessage"].asString());
        cout << " id = " << from << " send you msg:" << endl << chatMsg << endl;
        UserMap::iterator itr=userMap.find(from);
        if (itr==userMap.end()) return -1;
        itr->second->history << itr->second->nickname  << " (" << from << ") " << getTime() << ":\n" << chatMsg << "\n\n";
        emit s_recvChat(from);
    }
    //解析失败
    else {
        cout << errs << endl;
    }
    delete reader;
}

int Client::recvNotifiyLogin() {
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取状态码
    BYTE stateCode = recvData[5];
    if (stateCode == SEND) {
        Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
        Json::Value root;
        JSONCPP_STRING errs;
        bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
        //解析成功
        if (ok&&errs.size() == 0)
        {
            u_int accountId = root["accountId"].asUInt();
            if (userMap[accountId] != NULL) userMap[accountId]->isOnline = true;
            else {
                std::string nickname = UTF8_To_string(root["nickname"].asString());
                bool isOnline = root["isOnline"].asBool();
                userMap[accountId] = new User(accountId, nickname, isOnline);
            }
            emit s_recvNotifyLogin(accountId);
        }
        //解析失败
        else {
            cout << errs << endl;
        }
        delete reader;

    }
    else {
    }
}

int Client::recvLogout() {
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取状态码
    BYTE stateCode = recvData[5];
    if (stateCode == SEND) {
        Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
        Json::Value root;
        JSONCPP_STRING errs;
        bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
        //解析成功
        if (ok&&errs.size() == 0)
        {
            u_int logoutId = root["logoutId"].asUInt();
            if (logoutId==self.accountId) return 0;
            cout << logoutId << " logout" << endl;
            userMap[logoutId]->isOnline = false;
            emit s_recvLogout(logoutId);
        }
        //解析失败
        else {
            cout << errs << endl;
        }
        delete reader;

    }
    else {
    }
}

int Client::recvSrchFri() {
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取状态码
    BYTE stateCode = recvData[5];
    if (stateCode == OKAY) {
        Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
        Json::Value root, users;
        JSONCPP_STRING errs;
        bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
        //解析成功
        if (ok&&errs.size() == 0)
        {
            for (int i=0;i<srchfri.size();++i) delete srchfri[i];
            srchfri.clear();
            u_int accountId; bool isOnline; std::string nickname;
            for (u_int i = 0; i < root.size(); ++i) {
                accountId = root[i]["accountId"].asUInt();
                isOnline = root[i]["isOnline"].asBool();
                nickname = UTF8_To_string(root[i]["nickname"].asString());
                srchfri.push_back(new User(accountId, nickname, isOnline));
            }
            emit s_recvSrchFri(stateCode);
        }
        //解析失败
        else {
            cout << errs << endl;
        }
        delete reader;

    }
    else {
        cout << "Srch Fri failed" << endl;
    }
    emit s_recvLogin(stateCode);
}

int Client::recvAddFriRqst() {
    char* recvData = recvBuffer.getBufferHead();
    //读取数据包长度
    u_int recvDataLength = *(u_int*)recvData;
    //读取状态码
    BYTE stateCode = recvData[5];
    //接收到别人的好友请求
    if (stateCode == SEND) {
        Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
        Json::Value root, users;
        JSONCPP_STRING errs;
        bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
        //解析成功
        if (ok&&errs.size() == 0)
        {
            u_int fromId = root["fromId"].asUInt();
            u_int addId = root["addId"].asUInt();
            if (addId != self.accountId) {
                cout << "Recv add frirqst error, addId != selfId" << endl;
                return 0;
            }
            string comment = UTF8_To_string(root["comment"].asString());
            string fromName = UTF8_To_string(root["fromName"].asString());
            cout << "Recv add friend request from " << fromId << "(" << fromName << ") with coment = " << comment << endl;
            vAddFriRqst.push_back(AddFriRqst(fromId, fromName, comment));
        }
        //解析失败
        else {
            cout << errs << endl;
        }
        delete reader;
        emit s_recvAddFriRqst(stateCode);
    }
    //被接受为好友
    else if (stateCode == AGREE) {
        Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
        Json::Value root, users;
        JSONCPP_STRING errs;
        bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
        //解析成功
        if (ok&&errs.size() == 0)
        {
            u_int fromId = root["fromId"].asUInt();
            u_int addId = root["addId"].asUInt();
            if (fromId != self.accountId) {
                cout << "Recv add frirqst error, fromId != selfId" << endl;
                return 0;
            }
            string comment = UTF8_To_string(root["comment"].asString());
            string addName = UTF8_To_string(root["addName"].asString());
            cout << addName << "(" << addId << ") agree to add  " << fromId << " with coment = " << comment << endl;
        }
        //解析失败
        else {
            cout << errs << endl;
        }
        delete reader;
    }
    //被拒绝为好友
    else if (stateCode == REJECT) {
        Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
        Json::Value root, users;
        JSONCPP_STRING errs;
        bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
        //解析成功
        if (ok&&errs.size() == 0)
        {
            u_int fromId = root["fromId"].asUInt();
            u_int addId = root["addId"].asUInt();
            if (fromId != self.accountId) {
                cout << "Recv add frirqst error, fromId != selfId" << endl;
                return 0;
            }
            string comment = UTF8_To_string(root["comment"].asString());
            string addName = UTF8_To_string(root["addName"].asString());
            cout << addName << "(" << addId << ") reject to add  " << fromId << " with coment = " << comment << endl;
        }
        //解析失败
        else {
            cout << errs << endl;
        }
        delete reader;
    }
}
