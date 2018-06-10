#include "server.h"

Server::Server()
{
}

Server::~Server() {
    UserMap::iterator itr;
    for (itr=userMap.begin();itr!=userMap.end();++itr) delete itr->second;
}

bool Server::powerOn() {
    /***创建WSA***/
    WORD sockVersion = MAKEWORD(2, 2);

    /***检查WSA正确性***/
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        std::cout << "Error WSA!!!" << std::endl;
        return false;
    }

    /***创建监听socket***/
    slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /***创建监听socket失败***/
    if (slisten == INVALID_SOCKET) {
        std::cout << "Error getting socket!!!" << std::endl;
        return false;
    }

    /***绑定监听地址***/
    sin.sin_family = AF_INET;
    sin.sin_port = htons(LISTEN_PORT);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;

    /***绑定socket与监听地址***/
    if (bind(slisten, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
        std::cout << "Error binding!!!" << std::endl;
        return false;
    }

    /***开始监听***/
    if (listen(slisten, 5) == SOCKET_ERROR) {
        std::cout << "Error listening!!!" << std::endl;
        return false;
    }

    /***连接数据库***/
    database = QSqlDatabase::addDatabase("QSQLITE", "MyDataBase.db");
    database.setDatabaseName("MyDataBase.db");
    database.setUserName("root");
    database.setPassword("123456");
    /***数据库开启失败***/
    if (!database.open()) {
        qDebug() << "Error: Failed to connect database.\n" << database.lastError() << "\n";
        return false;
    }
    database.exec("SET NAMES 'Latin1'");
    QSqlQuery sql_query(database);
    QString create_sql = "create table account(accountId int primary key, nickname varchar(255), password varchar(255));";
    sql_query.prepare(create_sql);
    sql_query.exec();

    rwUserMap = CreateMutex(NULL, false, NULL);

    /***启动监听线程***/
    CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)listenThreadFunc, this, 0, NULL));

    /***成功启动服务器***/
    return true;
}

DWORD WINAPI listenThreadFunc(LPVOID lpParam) {
    Server* server = (Server*) lpParam;
    SOCKET& slisten = server->slisten;

    while (true) {
        SOCKET* sClient = new SOCKET;
        sockaddr_in* clientAddr = new sockaddr_in;
        printf("Wait for connection...\n");
        *sClient = accept(slisten, (sockaddr*)clientAddr, NULL);
        //接收错误
        if (*sClient == INVALID_SOCKET) {
            std::cout << WSAGetLastError() << std::endl;
            delete sClient; delete clientAddr;
            continue;
        }
        printf("receive connect from %s:%d\n", inet_ntoa(clientAddr->sin_addr), clientAddr->sin_port);
        //为接收的连接开启发送和接收线程
        MyBuffer* sendBuffer = new MyBuffer;
        HANDLE* rwSendBufferMutex = new HANDLE; *rwSendBufferMutex = CreateMutex(NULL, false, NULL);
        HANDLE* sendBufferSmp = new HANDLE; *sendBufferSmp = CreateSemaphore(NULL, 0, 10, NULL);
        RsParam* prsParam = new RsParam(server, sClient, clientAddr, sendBuffer, rwSendBufferMutex, sendBufferSmp);
        CloseHandle(CreateThread(NULL, 0, recvThreadFunc, prsParam, 0, NULL));
        CloseHandle(CreateThread(NULL, 0, sendThreadFunc, prsParam, 0, NULL));
    }
}

DWORD WINAPI recvThreadFunc(LPVOID lpParam) {
    //得到传递的参数
    RsParam* param = (RsParam*)lpParam;
    SOCKET* sClient = param->sClient;
    sockaddr_in* clientAddr = param->clientAddr;
    MyBuffer* sendBuffer = param->sendBuffer;
    HANDLE* rwSendBufferMutex = param->rwSendBufferMutex;
    HANDLE* sendBufferSmp = param->sendBufferSmp;
    Server* server = param->server;

    //接收缓冲区
    MyBuffer recvBuffer;
    char *recvData;
    BYTE actionCode;
    int ret = 0, dCnt = 0;
    u_int length;
    char ddd[1024];
    int nNetTimeout = 10000;
    //设置接受超时时间
    setsockopt(*sClient, SOL_SOCKET, SO_RCVTIMEO, (char*)&nNetTimeout, sizeof(int));
    while (true) {
        recvData = recvBuffer.getBufferEnd();//得到缓冲区尾部
        ret = recv(*sClient, recvData, recvBuffer.getRest(), 0);//接收数据
        cout << "recv ret=" << ret << endl;
        /***超过一定时间没有接收到客户端的响应，释放线程***/
        if (ret == -1) {
            if (dCnt++ < 3) {
                Sleep(300);
                continue;
            }
            std::cout << param->accountId << " exit" << std::endl;
            param->flag = true;
            break;
        }
        dCnt = 0;
        recvBuffer.setEnd(ret);
        //读取动作码
        recvData = recvBuffer.getBufferHead();
        length = *(u_int*)recvData;
        actionCode = *(BYTE*)(recvData+4);
        //判断行为
        switch (actionCode) {
        case HEARTBEAT: {
            break;
        }
        case LOGIN: {
            server->recvLogin(&recvBuffer, sendBuffer, rwSendBufferMutex, sendBufferSmp, param->accountId);
            std::cout << "LOGIN" << std::endl;
            break;
        }
        case CHAT: {
            server->recvChat(&recvBuffer, sendBuffer, rwSendBufferMutex, sendBufferSmp);
            break;
        }
        case REGISTER: {
            server->recvRegis(&recvBuffer, sendBuffer, rwSendBufferMutex, sendBufferSmp);
            std::cout << "REGISTER" << std::endl;
            break;
        }
        case SEARCH_ACCOUNT: {
            server->recvSrchFri(&recvBuffer, sendBuffer, rwSendBufferMutex, sendBufferSmp);
            break;
        }
        }
        recvBuffer.setHead(length);
    }
    cout << param->accountId << " exit!" << endl;
    UserMap::iterator itr = server->userMap.begin();
    //将数据段用json编码
    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["logoutId"] = param->accountId;
    /*可重用代码，产生整个报文*/
    std::string s = Json::writeString(wbuilder, root);
    for (itr;itr!=server->userMap.end();++itr) {
        server->fill_in(itr->first, s.c_str(), LOGOUT, SEND);
    }
    server->userMap.erase(param->accountId);
    return 0;
}

DWORD WINAPI sendThreadFunc(LPVOID lpParam) {
    RsParam* param = (RsParam*)lpParam;
    HANDLE* rwSendBufferMutex = param->rwSendBufferMutex;
    HANDLE* sendBufferSmp = param->sendBufferSmp;
    MyBuffer* sendBuffer = param->sendBuffer;
    SOCKET* sClient = param->sClient;
    Server* server = param->server;

    int ret = 0;//发送返回值
    char* sendData = NULL;//缓冲区指针
    u_int length = 0;//发送报文长度
    int dCnt = 0;

    int nNetTimeout = 10000;
    //设置接受超时时间
    setsockopt(*sClient, SOL_SOCKET, SO_SNDTIMEO, (char*)&nNetTimeout, sizeof(int));
    int tmp;
    while (true) {
        if (param->flag) break;
        WaitForSingleObject(*sendBufferSmp, INFINITE);
        //申请发送缓冲区
        WaitForSingleObject(*rwSendBufferMutex, INFINITE);
        //得到缓冲区头
        sendData = sendBuffer->getBufferHead();
        //得到发送报文长度
        length = *(u_int*)sendData;
        std::cout << "send data = " << sendData+6 << std::endl;
        ret = send(*sClient, sendData, length, 0);
        printf("socket = %d send ret = %d sendL = %u\n", *sClient, ret, length);
        if (ret == -1) {
            printf("send error!!!\n");
            ReleaseMutex(*rwSendBufferMutex);
            Sleep(300);
            continue;
        }
        sendBuffer->setHead(length);
        //释放互斥量
        ReleaseMutex(*rwSendBufferMutex);
    }
    delete (RsParam*)lpParam;
    delete sendBuffer;
    delete rwSendBufferMutex;
    delete sendBufferSmp;
    return 0;
}

int Server::fill_in(MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp, const char* sendMsg, BYTE actionCode, BYTE stateCode) {
    WaitForSingleObject(*rwSendBufferMutex, INFINITE);

    char* sendData = sendBuffer->getBufferEnd();
    u_int sendMsgLength = 0;
    if (sendMsg != NULL) sendMsgLength = strlen(sendMsg);
    *(u_int*)sendData = sendMsgLength+6;
    *(BYTE*)(sendData+4) = actionCode;
    *(BYTE*)(sendData+5) = stateCode;
    if (sendMsg != NULL) memccpy(sendData+6, sendMsg, 0, sendMsgLength);
    sendBuffer->setEnd(sendMsgLength+6);

    ReleaseMutex(*rwSendBufferMutex);
    ReleaseSemaphore(*sendBufferSmp, 1, NULL);
    return 0;
}

int Server::fill_in(u_int accountId, const char* sendMsg, BYTE actionCode, BYTE stateCode) {
    User* user = userMap[accountId];
    if (user==NULL) return -1;
    MyBuffer* sendBuffer = user->sendBuffer;
    HANDLE* rwSendBufferMutex = user->rwSendBufferMutex;
    HANDLE* sendBufferSmp = user->sendBufferSmp;

    WaitForSingleObject(*rwSendBufferMutex, INFINITE);

    char* sendData = sendBuffer->getBufferEnd();
    std::cout << "get end = " << (int)sendData << std::endl;
    u_int sendMsgLength = 0;
    if (sendMsg != NULL) sendMsgLength = strlen(sendMsg);
    *(u_int*)sendData = sendMsgLength+6;
    std::cout << *(u_int*)sendData << std::endl;
    *(BYTE*)(sendData+4) = actionCode;
    *(BYTE*)(sendData+5) = stateCode;
    if (sendMsg != NULL) memccpy(sendData+6, sendMsg, 0, sendMsgLength);
    sendBuffer->setEnd(sendMsgLength+6);

    ReleaseMutex(*rwSendBufferMutex);
    ReleaseSemaphore(*sendBufferSmp, 1, NULL);
    return 0;
}

int Server::recvLogin(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp, u_int& aId) {
    char* recvData = recvBuffer->getBufferHead();
    u_int recvDataLength = *(u_int*)(recvData);
    Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
    Json::Value root;
    JSONCPP_STRING errs;
    bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
    //解析成功
    if (ok&&errs.size() == 0)
    {
        u_int accountId = root["accountId"].asUInt();    // 得到帐户id
        string pwd = root["password"].asString();	//得到帐户密码
        printf("Login with account = %d, password = %s\n", accountId, pwd.c_str());
        /*验证合法性*/
        bool validate = false;
        QSqlQuery query(database);
        QString statement = QString("select * from account where accountId = %1 and password = '%2'").arg(accountId).arg(pwd.c_str());
        query.exec(statement);
        if (query.next()) {
            validate = true;
        }

        //检验登陆信息合法性
        if (!validate) {
            fill_in(sendBuffer, rwSendBufferMutex, sendBufferSmp, NULL, LOGIN, ERR);
            std::cout << "Login failed" << std::endl;
        }
        //登陆验证成功
        else {
            string n = query.value("nickname").toString().toStdString();
            WaitForSingleObject(rwUserMap, INFINITE);
            userMap[accountId] = new User(accountId, n, recvBuffer, sendBuffer, rwSendBufferMutex, sendBufferSmp);
            ReleaseMutex(rwUserMap);
            aId = accountId;
            //将数据段用json编码
            Json::Value root;
            Json::Value users;
            Json::StreamWriterBuilder wbuilder;

            UserMap::iterator itr;
            for (itr = userMap.begin(); itr != userMap.end(); ++itr) {
                if (itr->first == accountId) continue;
                users.clear();
                users["accountId"] = itr->second->accountId;
                users["nickname"] = itr->second->nickname;
                users["isOnline"] = false;
                root["friend"].append(users);
            }
            root["accountId"] = accountId;
            root["nickname"] = n;
            /*可重用代码，产生整个报文*/
            std::string s = Json::writeString(wbuilder, root);

            if (fill_in(accountId, s.c_str(), LOGIN, OKAY)!=-1)
                std::cout << "Login okay" << std::endl;
            else
                std::cout << "Login failed 2" << std::endl;

            //通知线上的所有人
            Json::Value root_t;
            root_t["accountId"] = accountId;
            root_t["nickname"] = n;
            s = Json::writeString(wbuilder, root_t);
            for (itr = userMap.begin(); itr != userMap.end(); ++itr) {
                if (itr->first!=accountId) {
                    fill_in(itr->first, s.c_str(), NOTIFY_LOGIN, SEND);
                    std::cout << "Notify " << itr->first << std::endl;
                }
            }
        }
    }
    //解析失败
    else {
        std::cout << errs << "\n";
    }
    delete reader;
}

int Server::recvRegis(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp) {
    char* recvData = recvBuffer->getBufferHead();
    u_int recvDataLength = *(u_int*)(recvData);
    Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
    Json::Value root;
    JSONCPP_STRING errs;
    bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
    //解析成功
    if (ok&&errs.size() == 0)
    {
        u_int accountId = root["accountId"].asUInt();    // 得到帐户id
        string nickname = root["nickname"].asString();	//得到帐户昵称
        string password = root["password"].asString();	//得到帐户密码
        printf("%d register with nickname = %s and pwd = %s\n", accountId, nickname.c_str(), password.c_str());

        QSqlQuery query(database);
        QString statement = QString("insert into account values(%1, '%2', '%3');").arg(accountId).arg(nickname.c_str()).arg(password.c_str());
        qDebug() << statement;

        if (!query.exec(statement)) {
            qDebug() << query.lastError();
            fill_in(sendBuffer, rwSendBufferMutex, sendBufferSmp, NULL, REGISTER, ERR);
        }
        else {
            qDebug() << "Insert done\n";
            fill_in(sendBuffer, rwSendBufferMutex, sendBufferSmp, NULL, REGISTER, OKAY);
        }
    }
    //解析失败
    else {
        cout << errs << endl;
    }
    delete reader;
}

int Server::recvChat(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp) {
    char* recvData = recvBuffer->getBufferHead();
    u_int recvDataLength = *(u_int*)(recvData);
    Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
    Json::Value root;
    JSONCPP_STRING errs;
    bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
    //解析成功
    if (ok&&errs.size() == 0)
    {
        u_int friendAccountId = root["friendAccountId"].asUInt();    // 得到帐户id
        u_int accountId = root["accountId"].asUInt();	//得到帐户昵称
        string chatMessage = root["chatMessage"].asString();	//得到帐户密码

        //将数据段用json编码
        Json::Value root;
        Json::StreamWriterBuilder wbuilder;

        root["fromAccountId"] = accountId;
        root["chatMessage"] = chatMessage;
        /*可重用代码，产生整个报文*/
        std::string s = Json::writeString(wbuilder, root);

        fill_in(friendAccountId, s.c_str(), CHAT, SEND);
    }
    //解析失败
    else {
        cout << errs << endl;
    }
    delete reader;
}

int Server::recvSrchFri(MyBuffer* recvBuffer, MyBuffer* sendBuffer, HANDLE* rwSendBufferMutex, HANDLE* sendBufferSmp) {
    char* recvData = recvBuffer->getBufferHead();
    u_int recvDataLength = *(u_int*)(recvData);
    Json::CharReader* reader = Json::CharReaderBuilder().newCharReader();
    Json::Value root;
    JSONCPP_STRING errs;
    bool ok = reader->parse(recvData + 6, recvData + recvDataLength, &root, &errs);
    //解析成功
    if (ok&&errs.size() == 0)
    {
        string search_content = root["search_content"].asString();

        QSqlQuery query(database);
        QString statement = QString("select * from account where nickname like '%%2%'").arg(search_content.c_str());
        qDebug() << statement;
        query.exec(statement);
        Json::Value root, u;
        Json::StreamWriterBuilder wbuilder;
        u_int aId;
        while (query.next()) {
            u.clear();
            aId = query.value("accountId").toUInt();
            u["accountId"] = aId;
            u["nickname"] = query.value("nickname").toString().toStdString();
            if (userMap.find(aId)!=userMap.end()) u["isOnline"] = true;
            else u["isOnline"] = false;
            cout << u << "\n";
            root.append(u);
        }
        std::string s = Json::writeString(wbuilder, root);
        fill_in(sendBuffer, rwSendBufferMutex, sendBufferSmp, s.c_str(), SEARCH_ACCOUNT, OKAY);
    }
    //解析失败
    else {
        cout << errs << endl;
    }
    delete reader;
}
