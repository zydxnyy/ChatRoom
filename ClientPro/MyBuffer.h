#ifndef MYBUFFER_H
#define MYBUFFER_H

#include <iostream>
#include <stdio.h>
#include <windows.h>
using namespace std;

#define MAX_BUFFER_LENGTH 16*1024
typedef unsigned int u_int;

class  MyBuffer {
	HANDLE setMutex;
		char* buffer;
		u_int curHead;
		u_int curEnd;

	public:
		MyBuffer();
		~MyBuffer();
		char* getBufferHead() const;
		char* getBufferEnd() const;
		void setHead(u_int offset);
		bool setEnd(u_int offset);
		u_int getRest() const;
};

#endif
