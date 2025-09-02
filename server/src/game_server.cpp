#include "globals.h"
#include "echo_service.h"
#include "space_service.h"
#include "wheel_timer.h"
#include "physics/physics_mgr.h"

#include <google/protobuf/stubs/common.h>
#include <event2/event.h>
#include <spdlog/spdlog.h>

struct event_base* EVENT_BASE = nullptr;
std::string DATA_PATH;

WheelTimer G_Timer{ 33, 1024 };
PhysicsMgr G_PhysicsMgr;

const char* HOST = "0.0.0.0";
const int PORT = 1988;

static void on_libevent_update(evutil_socket_t fd, short what, void* arg)
{
    G_Timer.update();
}

void init_timer() {
    struct event* ev = event_new(EVENT_BASE, -1, EV_PERSIST, on_libevent_update, nullptr);
    struct timeval tv = { 0, 33 * 1000 };
    event_add(ev, &tv);

    G_Timer.start();
}

int main(int argc, char** argv) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    spdlog::set_level(spdlog::level::debug);

    // 资源路径
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--data") {
            if (i + 1 < argc) {
                DATA_PATH = argv[++i];
            }
            else {
                spdlog::error("--data requires a path argument!");
                return 1;
            }
        }
    }

    if (DATA_PATH.empty()) {
        spdlog::error("need specify data path, use --data arguemnt");
        return 1;
    }

    int ret = G_PhysicsMgr.init();
    if (0 != ret) {
        spdlog::error("init_physx failed!");
        return ret;
    }

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(0x0201, &wsa_data) != 0) {
        DWORD error = WSAGetLastError();
        LPSTR buffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);
        std::string message(buffer, size);
        LocalFree(buffer);
        spdlog::error("WSAStartup failed: {}", message);
        return 1;
    }
#endif

    EVENT_BASE = event_base_new();
    if (!EVENT_BASE) {
        spdlog::error("create event base failed!");
        return -1;
    }
    init_timer();

    spdlog::info("game server started!");

    SpaceService space_service;
    space_service.start(HOST, PORT);

    event_base_dispatch(EVENT_BASE);

    G_PhysicsMgr.fini();
    return 0;
}
