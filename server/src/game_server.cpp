#include "globals.h"
#include "network/tcp_server.h"

#include <event2/event.h>

#include <iostream>

struct event_base* EVENT_BASE = nullptr;

const char* HOST = "0.0.0.0";
const int PORT = 1988;

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(0x0201, &wsa_data) != 0) {
        DWORD error = WSAGetLastError();
        LPSTR buffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);
        std::string message(buffer, size);
        LocalFree(buffer);
        std::cerr << "WSAStartup failed: " << message << std::endl;
        return 1;
    }
#endif

    EVENT_BASE = event_base_new();
    if (!EVENT_BASE) {
        std::cerr << "create event base failed!" << std::endl;
        return -1;
    }

    TcpServer tcp_server(HOST, PORT);
    tcp_server.start(EVENT_BASE);

    event_base_dispatch(EVENT_BASE);
    return 0;
}
