#include "MyBuffer.h"

MyBuffer::MyBuffer() {
    buffer = new char [MAX_BUFFER_LENGTH];
    curHead =  curEnd = 0;
}

MyBuffer::~MyBuffer() {
    delete [] buffer;
    buffer = NULL;
}

char* MyBuffer::getBufferHead() const{
    char* ret = buffer + curHead;
    return ret;
}

char* MyBuffer::getBufferEnd() const{
    char* ret = buffer + curEnd;
    return ret;
}

u_int MyBuffer::getRest() const{
    int rest = MAX_BUFFER_LENGTH - curEnd;
    return rest;
}

bool MyBuffer::setEnd(u_int offset) {
    curEnd += offset;
    *(buffer+curEnd) = 0;
    curEnd += 1;
    if (curEnd > MAX_BUFFER_LENGTH) {
        curEnd -= offset;
        return false;
    }
    return true;
}

void MyBuffer::setHead(u_int offset) {
    //如果已经把缓冲区的数据读取完毕，清空
    if (curHead+offset+1 == curEnd) {
        curHead = curEnd= 0;
    }
    //读取错误，抛弃这一个数据包
    else if (curHead+offset+1 > curEnd) {
        cout << "You have read exceed the data with head = " << curHead << " end = " << curEnd << "\n";
        curHead += 6;
        while (buffer[curHead++]!=0);
        if (curHead >= curEnd) {
            curHead = curEnd= 0;
        }
        return;
    }
    else curHead += offset + 1;
}
