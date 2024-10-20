#pragma once

#include "service.h"

#include <map>
#include <set>
#include <string>

class TcpConnection;

class Space;
class Player;

class SpaceService : public Service {
public:
    using MsgHandlerFunc = void (SpaceService::*)(TcpConnection*, const std::string&);
    static std::map<std::string, MsgHandlerFunc> name_2_handler;
    static std::map<std::string, MsgHandlerFunc> register_msg_handlers();
    static MsgHandlerFunc find_msg_handler(const std::string& msg_name);

    ~SpaceService() {}
    virtual void start(const std::string& listen_address, int listen_port) override;
    virtual void stop() override;

    virtual void on_lost_connection(TcpConnection* conn) override;
    virtual void handle_msg(TcpConnection* conn, const std::string& msg) override;

    // 请求以username登录
    void login(TcpConnection*, const std::string&);

    // 请求进入场景（此时客户端场景已加载完成）
    void join(TcpConnection*, const std::string&);
    // 请求离开场景
    void leave(TcpConnection*, const std::string&);

    Player* find_player(TcpConnection* conn);

private:
    Space* _space = nullptr;

    std::map<TcpConnection*, Player*> _conn_2_player;
    std::set<std::string> _exists_names;
};
