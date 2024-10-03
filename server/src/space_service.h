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
    virtual void start(const std::string& listen_address, int listen_port) override;
    virtual void stop() override;

    virtual void on_lost_connection(TcpConnection* conn) override;
    virtual void handle_msg(TcpConnection* conn, const std::string& msg) override;

    // 请求以username进入场景
    void join(TcpConnection* conn, const std::string& username);
    // 请求离开场景
    void leave(TcpConnection* conn);

    Player* find_player(TcpConnection* conn);

private:
    Space* _space = nullptr;

    std::map<TcpConnection*, Player*> _conn_2_player;
    std::set<std::string> _exists_names;
};
