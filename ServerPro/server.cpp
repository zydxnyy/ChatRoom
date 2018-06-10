#include "server.h"

Server::Server()
{
}

Server::~Server() {
    UserMap::iterator itr;
    for (itr=userMap.begin();itr!=userMap.end();++itr) delete itr->second;
}

bool Server::powerOn() {
    /***����WSA***/
    WORD sockVersion = MAKEWORD(2, 2);

    /***���WSA��ȷ��***/
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        std::cout << "Error WSA!!!" << std::endl;
        return false;
    }

    /***��������socket***/
    slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /***��������socketʧ��***/
    if (slisten == INVALID_SOCKET) {
        std::cout << "Error getting socket!!!" << std::endl;
        return false;
    }

    /***�󶨼�����ַ***/
    sin.sin_family = AF_INET;
    sin.sin_port = htons(LISTEN_PORT);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;

    /***��socket�������ַ***/
    if (bind(slisten, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
        std::cout << "Error binding!!!" << std::endl;
        return false;
    }

    /***��ʼ����***/
    if (listen(slisten, 5) == SOCKET_ERROR) {
        std::cout << "Error listening!!!" << std::endl;
        return false;
    }

    /***�������ݿ�***/
    database = QSqlDatabase::addDatabase("QSQLITE", "MyDataBase.db");
    database.setDatabaseName("MyDataBase.db");
    database.setUserName("root");
    database.setPassword("123456");
    /***���ݿ⿪��ʧ��***/
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

    /***���������߳�***/
    CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)listenThreadFunc, this, 0, NULL));

    /***�ɹ�����������***/
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
        //���մ���
        if (*sClient == INVALID_SOCKET) {
            std::cout << WSAGetLastError() << std::endl;
            delete sClient; delete clientAddr;
            continue;
        }
        printf("receive connect from %s:%d\n", inet_ntoa(clientAddr->sin_addr), clientAddr->sin_port);
        //Ϊ���յ����ӿ������ͺͽ����߳�
        MyBuffer* sendBuffer = new MyBuffer;
        HANDLE* rwSendBufferMutex = new HANDLE; *rwSendBufferMutex = CreateMutex(NULL, false, NULL);
        HANDLE* sendBufferSmp = new HANDLE; *sendBufferSmp = CreateSemaphore(NULL, 0, 10, NULL);
        RsParam* prsParam = new RsParam(server, sClient, clientAddr, sendBuffer, rwSendBufferMutex, sendBufferSmp);
        CloseHandle(CreateThread(NULL, 0, recvThreadFunc, prsParam, 0, NULL));
        CloseHandle(CreateThread(NULL, 0, sendThreadFunc, prsParam, 0, NULL));
    }
}

DWORD WINAPI recvThreadFunc(LPVOID lpParam) {
    //�õ����ݵĲ���
    RsParam* param = (RsParam*)lpParam;
    SOCKET* sClient = param->sClient;
    sockaddr_in* clientAddr = param->clientAddr;
    MyBuffer* sendBuffer = param->sendBuffer;
    HANDLE* rwSendBufferMutex = param->rwSendBufferMutex;
    HANDLE* sendBufferSmp = param->sendBufferSmp;
    Server* server = param->server;

    //���ջ�����
    MyBuffer recvBuffer;
    char *recvData;
    BYTE actionCode;
    int ret = 0, dCnt = 0;
    u_int length;
    char ddd[1024];
    int nNetTimeout = 10000;
    //���ý��ܳ�ʱʱ��
    setsockopt(*sClient, SOL_SOCKET, SO_RCVTIMEO, (char*)&nNetTimeout, sizeof(int));
    while (true) {
        recvData = recvBuffer.getBufferEnd();//�õ�������β��
        ret = recv(*sClient, recvData, recvBuffer.getRest(), 0);//��������
        cout << "recv ret=" << ret << endl;
        /***����һ��ʱ��û�н��յ��ͻ��˵���Ӧ���ͷ��߳�***/
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
        //��ȡ������
        recvData = recvBuffer.getBufferHead();
        length = *(u_int*)recvData;
        actionCode = *(BYTE*)(recvData+4);
        //�ж���Ϊ
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
    //�����ݶ���json����
    Json::Value root;
    Json::StreamWriterBuilder wbuilder;
    root["logoutId"] = param->accountId;
    /*�����ô��룬������������*/
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

    int ret = 0;//���ͷ���ֵ
    char* sendData = NULL;//������ָ��
    u_int length = 0;//���ͱ��ĳ���
    int dCnt = 0;

    int nNetTimeout = 10000;
    //���ý��ܳ�ʱʱ��
    setsockopt(*sClient, SOL_SOCKET, SO_SNDTIMEO, (char*)&nNetTimeout, sizeof(int));
    int tmp;
    while (true) {
        if (param->flag) break;
        WaitForSingleObject(*sendBufferSmp, INFINITE);
        //���뷢�ͻ�����
        WaitForSingleObject(*rwSendBufferMutex, INFINITE);
        //�õ�������ͷ
        sendData = sendBuffer->getBufferHead();
        //�õ����ͱ��ĳ���
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
        //�ͷŻ�����
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
    //�����ɹ�
    if (ok&&errs.size() == 0)
    {
        u_int accountId = root["accountId"].asUInt();    // �õ��ʻ�id
        string pwd = root["password"].asString();	//�õ��ʻ�����
        printf("Login with account = %d, password = %s\n", accountId, pwd.c_str());
        /*��֤�Ϸ���*/
        bool validate = false;
        QSqlQuery query(database);
        QString statement = QString("select * from account where accountId = %1 and password = '%2'").arg(accountId).arg(pwd.c_str());
        query.exec(statement);
        if (query.next()) {
            validate = true;
        }

        //�����½��Ϣ�Ϸ���
        if (!validate) {
            fill_in(sendBuffer, rwSendBufferMutex, sendBufferSmp, NULL, LOGIN, ERR);
            std::cout << "Login failed" << std::endl;
        }
        //��½��֤�ɹ�
        else {
            string n = query.value("nickname").toString().toStdString();
            WaitForSingleObject(rwUserMap, INFINITE);
            userMap[accountId] = new User(accountId, n, recvBuffer, sendBuffer, rwSendBufferMutex, sendBufferSmp);
            ReleaseMutex(rwUserMap);
            aId = accountId;
            //�����ݶ���json����
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
            /*�����ô��룬������������*/
            std::string s = Json::writeString(wbuilder, root);

            if (fill_in(accountId, s.c_str(), LOGIN, OKAY)!=-1)
                std::cout << "Login okay" << std::endl;
            else
                std::cout << "Login failed 2" << std::endl;

            //֪ͨ���ϵ�������
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
    //����ʧ��
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
    //�����ɹ�
    if (ok&&errs.size() == 0)
    {
        u_int accountId = root["accountId"].asUInt();    // �õ��ʻ�id
        string nickname = root["nickname"].asString();	//�õ��ʻ��ǳ�
        string password = root["password"].asString();	//�õ��ʻ�����
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
    //����ʧ��
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
    //�����ɹ�
    if (ok&&errs.size() == 0)
    {
        u_int friendAccountId = root["friendAccountId"].asUInt();    // �õ��ʻ�id
        u_int accountId = root["accountId"].asUInt();	//�õ��ʻ��ǳ�
        string chatMessage = root["chatMessage"].asString();	//�õ��ʻ�����

        //�����ݶ���json����
        Json::Value root;
        Json::StreamWriterBuilder wbuilder;

        root["fromAccountId"] = accountId;
        root["chatMessage"] = chatMessage;
        /*�����ô��룬������������*/
        std::string s = Json::writeString(wbuilder, root);

        fill_in(friendAccountId, s.c_str(), CHAT, SEND);
    }
    //����ʧ��
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
    //�����ɹ�
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
    //����ʧ��
    else {
        cout << errs << endl;
    }
    delete reader;
}
