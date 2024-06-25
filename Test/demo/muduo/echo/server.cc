#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <spdlog/spdlog.h>

using namespace muduo;
using namespace muduo::net;

class EchoServer
{
private:
    EventLoop _loop;
    TcpServer _server;
public:
    EchoServer(const InetAddress& lisntenAddr)
    :_server(&_loop, lisntenAddr, "EchoServer")
    {
        _server.setConnectionCallback(std::bind(&EchoServer::OnConnection, this, _1));
        _server.setMessageCallback(std::bind(&EchoServer::OnMessage, this, _1, _2, _3));
    }

    void Start()
    {
        _server.start();
        _loop.loop();
    }

private:
    void OnConnection(const TcpConnectionPtr& conn)
    {
        spdlog::info("{} -> {} is {}", conn->peerAddress().toIpPort(), 
        conn->localAddress().toIpPort(), conn->connected() ? "UP" : "DOWN");
    }

    void OnMessage(const TcpConnectionPtr& conn, Buffer*  buf, Timestamp time)
    {
        string msg(buf->retrieveAllAsString());
        spdlog::info("{} recv:{} bytes at {}", conn->name(), msg.size(), time.toString());
        conn->send(msg);
    }
};

int main()
{
    EchoServer server(InetAddress(8888));
    server.Start();

    return 0;
}