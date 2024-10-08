#include "space_service.h"
#include "space.h"
#include "player.h"

#include "network/tcp_connection.h"

void SpaceService::start(const std::string& listen_address, int listen_port)
{
    Service::start(listen_address, listen_port);

    // 提供一个场景
    _space = new Space(30, 30);
}

void SpaceService::stop()
{
    delete _space;
    _space = nullptr;

    // 连接由tcp_server管理，这里只管干掉player就好
    for (auto& [_, player] : _conn_2_player) {
        delete player;
    }

    Service::stop();
}

void SpaceService::on_lost_connection(TcpConnection* conn)
{
    leave(conn);
    delete conn;
}

void SpaceService::handle_msg(TcpConnection* conn, const std::string& msg)
{
    if (msg.starts_with("login#")) {
        std::string username = msg.substr(5);
        login(conn, username);
    }
    else if (msg.starts_with("join")) {
        join(conn);
    }
    else if (msg.starts_with("leave")) {
        leave(conn);
    }
}

void SpaceService::login(TcpConnection* conn, const std::string& username)
{
    if (find_player(conn)) {
        return;
    }

    if (_exists_names.contains(username)) {
        const char* err = "login_reply#1";
        conn->send_msg(err, strlen(err));
        return;
    }

    Player* player = new Player{ conn, username };
    _conn_2_player.insert(std::make_pair(conn, player));
    _exists_names.insert(username);

    const char* login_reply = "login_reply#0";
    player->send_msg(login_reply, strlen(login_reply));
}

void SpaceService::join(TcpConnection* conn)
{
    Player* player = find_player(conn);
    if (!player)
        return;

    _space->join(player);
}

void SpaceService::leave(TcpConnection* conn)
{
    Player* player = find_player(conn);
    if (!player)
        return;

    _space->leave(player);

    _conn_2_player.erase(conn);
    _exists_names.erase(player->get_name());

    delete player;
}

Player* SpaceService::find_player(TcpConnection* conn)
{
    auto iter = _conn_2_player.find(conn);
    return iter == _conn_2_player.end() ? nullptr : iter->second;
}
