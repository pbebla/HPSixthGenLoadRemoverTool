#include "LiveSplitServer.h"
LiveSplitServer::LiveSplitServer() {
    isLoading = false;
}

BOOL LiveSplitServer::open_pipe()
{
    pipe = CreateFile(L"//./pipe/LiveSplit", GENERIC_READ | GENERIC_WRITE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    return pipe != INVALID_HANDLE_VALUE; 
}

int LiveSplitServer::send_to_ls(std::string message)
{
    const char* sendbuf = message.c_str();
    DWORD cbWritten;
    BOOL fSuccess = WriteFile(pipe, sendbuf, strlen(sendbuf), &cbWritten, NULL);
    return 0;
}

void LiveSplitServer::close_pipe()
{
    CloseHandle(pipe);
}

bool LiveSplitServer::get_isLoading() {
    return isLoading;
}

void LiveSplitServer::set_isLoading(bool loading) {
    isLoading = loading;
}

