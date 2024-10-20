#include "space_service.h"
#include "space.h"
#include "player.h"

#include "bit_utils.h"

#include "network/tcp_connection.h"

#include "proto/space_service.pb.h"

#define REG_MSG_HANDLER(msg_name) name_2_handler.insert(std::make_pair(#msg_name, &SpaceService::##msg_name))

enum class LoginError {
    already_logined = 1,
    name_exists = 2
};

std::map<std::string, SpaceService::MsgHandlerFunc> SpaceService::name_2_handler = SpaceService::register_msg_handlers();

std::map<std::string, SpaceService::MsgHandlerFunc> SpaceService::register_msg_handlers()
{
    std::map<std::string, MsgHandlerFunc> name_2_handler;
    REG_MSG_HANDLER(login);
    REG_MSG_HANDLER(join);
    REG_MSG_HANDLER(leave);
    return name_2_handler;
}

SpaceService::MsgHandlerFunc SpaceService::find_msg_handler(const std::string& msg_name)
{
    auto iter = name_2_handler.find(msg_name);
    return iter != name_2_handler.end() ? iter->second : nullptr;
}

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
    leave(conn, std::string{});
    delete conn;
}

void SpaceService::handle_msg(TcpConnection* conn, const std::string& msg)
{
    int i = 0;
    // 每条msg由消息名称和消息参数构成
    const char* bytes = msg.c_str();
    // 消息名称字符串长度，以7bit的方式编码
    uint32_t msg_name_lenght = read_7bit_encoded_int(bytes, &i);
    // 消息名称字符串
    size_t pos = (size_t)i;
    std::string msg_name = msg.substr(pos, msg_name_lenght);
    // 消息参数
    pos += (size_t)msg_name_lenght;
    std::string msg_bytes = msg.substr(pos);

    MsgHandlerFunc msg_handler = find_msg_handler(msg_name);
    if (msg_handler) {
        (this->*msg_handler)(conn, msg_bytes);
    }
    else {
        std::cerr << "msg_handler not found: " << msg_name << std::endl;
    }
}

void SpaceService::login(TcpConnection* conn, const std::string& msg_bytes)
{
    space_service::LoginRequest login_req;
    login_req.ParseFromString(msg_bytes);

    const std::string& username = login_req.username();
    std::cout << "login: " << username << std::endl;

    space_service::LoginReply login_reply;
    int result = 0;
    if (find_player(conn)) {
        result = (int)LoginError::already_logined;
    }
    else if (_exists_names.contains(username)) {
        result = (int)LoginError::name_exists;
    }
    else {
        Player* player = new Player{ conn, username };
        _conn_2_player.insert(std::make_pair(conn, player));
        _exists_names.insert(username);
    }

    login_reply.set_result(result);
    send_proto_msg(conn, "login_reply", login_reply);
}

void SpaceService::join(TcpConnection* conn, const std::string&)
{
    Player* player = find_player(conn);
    if (!player)
        return;

    _space->join(player);
}

void SpaceService::leave(TcpConnection* conn, const std::string&)
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
