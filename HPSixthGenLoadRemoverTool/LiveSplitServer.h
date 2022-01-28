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
#define DEFAULT_PORT "16834"
class LiveSplitServer
{
private:
	WSADATA wsaData;
	SOCKET ConnectSocket;
	int iResult;
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	bool isLoading;
public:
	LiveSplitServer();
	void try_connection();
	int ls_connect();
	int send_to_ls(std::string message);
	int ls_close();
	bool get_isLoading();
	void set_isLoading(bool);
	
};

