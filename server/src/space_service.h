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
    ~SpaceService() {}
    virtual void start(const std::string& listen_address, int listen_port) override;
    virtual void stop() override;

    virtual void on_lost_connection(TcpConnection* conn) override;
    virtual void handle_msg(TcpConnection* conn, const std::string& msg) override;

    // 请求以username登录
    void login(TcpConnection* conn, const std::string& username);
    // 请求进入场景（此时客户端场景已加载完成）
    void join(TcpConnection* conn);
    // 请求离开场景
    void leave(TcpConnection* conn);

    Player* find_player(TcpConnection* conn);

private:
    Space* _space = nullptr;

    std::map<TcpConnection*, Player*> _conn_2_player;
    std::set<std::string> _exists_names;
};
