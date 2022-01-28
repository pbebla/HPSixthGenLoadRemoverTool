#include "LiveSplitServer.h"
LiveSplitServer::LiveSplitServer() {

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        throw std::exception("WSAStartup failed: %d\n", iResult);
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    // Resolve the server address and port
    iResult = getaddrinfo("10.0.0.38", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        WSACleanup();
        throw std::exception("getaddrinfo failed: %d\n", iResult);
    }

    ConnectSocket = INVALID_SOCKET;
    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    ptr = result;

    // Create a SOCKET for connecting to server
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
        ptr->ai_protocol);

    if (ConnectSocket == INVALID_SOCKET) {
        freeaddrinfo(result);
        WSACleanup();
        throw std::exception("Error at socket(): %ld\n", WSAGetLastError());
    }
    isLoading = false;
}

void LiveSplitServer::try_connection()
{
    // Connect to server.
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    // Should really try the next address returned by getaddrinfo
    // if the connect call failed
    // But for this simple example we just free the resources
    // returned by getaddrinfo and print an error message

    freeaddrinfo(result);

    
}

int LiveSplitServer::ls_connect() {
    std::thread thread_obj(&LiveSplitServer::try_connection, this);
    thread_obj.join();
    if (ConnectSocket == INVALID_SOCKET) {
        OutputDebugStringW(L"Unable to connect to server\n");
        WSACleanup();
        return 1;
    }
    OutputDebugStringW(L"Connected to server\n");
    return 0;

}

int LiveSplitServer::send_to_ls(std::string message)
{

    int recvbuflen = DEFAULT_BUFLEN;

    const char* sendbuf = message.c_str();
    char recvbuf[DEFAULT_BUFLEN];

    // Send an initial buffer
    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR) {
        wchar_t text_buffer[20] = { 0 }; //temporary buffer
        swprintf(text_buffer, _countof(text_buffer), L"send failed: %d\n", WSAGetLastError());
        OutputDebugStringW(text_buffer);
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    wchar_t text_buffer[20] = { 0 }; //temporary buffer
    swprintf(text_buffer, _countof(text_buffer), L"Bytes Sent: %ld\n", iResult);
    OutputDebugStringW(text_buffer);
    return 0;

}

int LiveSplitServer::ls_close()
{
    // shutdown the connection for sending since no more data will be sent
    // the client can still use the ConnectSocket for receiving data
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        wchar_t text_buffer[20] = { 0 }; //temporary buffer
        swprintf(text_buffer, _countof(text_buffer), L"shutdown failed: %d\n", WSAGetLastError());
        OutputDebugStringW(text_buffer);
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    OutputDebugStringW(L"Disconnected to server\n");
    return 0;
}

bool LiveSplitServer::get_isLoading() {
    return isLoading;
}

void LiveSplitServer::set_isLoading(bool loading) {
    isLoading = loading;
}