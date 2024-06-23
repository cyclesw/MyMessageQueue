#include "request.pb.h"
#include "dispatcher.h"
#include "codec.h"
#include <spdlog/spdlog.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <functional>

using namespace muduo;
using namespace muduo::net;
using namespace MyMQ;

class Server
{
private:
    EventLoop _loop;
    TcpServer _server;
    ProtobufCodec _codec;
    ProtobufDispatcher _dispatcher;
public:
    using PeopleInfoPtr = std::shared_ptr<PeopleInfo>;

    Server(int port)
    :_server(&_loop, InetAddress(port), "Server"),
     _dispatcher(std::bind(&Server::OnUnknowndMessage, this, _1, _2, _3)),
     _codec(std::bind(&ProtobufDispatcher::onProtobufMessage, &_dispatcher, _1, _2, _3))
    {
        _dispatcher.registerMessageCallback<PeopleInfo>(std::bind(&Server::OnMessage, this, _1, _2, _3));

        _server.setConnectionCallback(std::bind(&Server::OnConnection, this, _1));
        _server.setMessageCallback(std::bind(&ProtobufCodec::onMessage, &_codec, _1, _2, _3));
    }
private:
    void OnUnknowndMessage(const TcpConnectionPtr& conn, const MessagePtr& msg, Timestamp)
    {
        spdlog::info("Unkown Message from:{}", conn->peerAddress().toIpPort());
    }

    void OnMessage(const TcpConnectionPtr& conn, const PeopleInfoPtr& msg, Timestamp)
    {
        spdlog::info("[{}]:{}", msg->info().id(), msg->msg());
    }

    void OnConnection(const TcpConnectionPtr& conn)
    {
        spdlog::info("{} -> {} is {}", conn->peerAddress().toIpPort(), 
        conn->localAddress().toIpPort(), conn->connected() ? "UP" : "DOWN");
    }

};

int main()
{
    return 0;
}