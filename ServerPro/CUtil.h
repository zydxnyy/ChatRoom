#ifndef CUTIL_H
#define CUTIL_H

#include <Windows.h>
#include <string.h>
#include <string>
using namespace std;

std::string string_To_UTF8(const std::string & str);
std::string UTF8_To_string(const std::string & str);

#endif
