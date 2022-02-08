#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <exception>
#include <thread>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
class LiveSplitServer
{
private:
	bool isLoading;
	HANDLE pipe;
public:
	LiveSplitServer();
	BOOL open_pipe();
	int send_to_ls(std::string message);
	void close_pipe();
	bool get_isLoading();
	void set_isLoading(bool);
	
};

