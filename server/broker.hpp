#pragma once

#include "channel.hpp"
#include "consumer.hpp"
#include "log.hpp"
#include "codec.h"
#include "dispatcher.h"
#include "connection.hpp"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

namespace MyMQ
{
    #define DBFILE "meta.db"
    #define HOSTNAME "MyVHost"

    using namespace muduo::net;

//    using MyMessagePtr = std::shared_ptr<google::protobuf::Message>;

    class Server {
    private:
        EventLoop _baseloop;
        TcpServer _server;
        ProtobufDispatcher _dispatcher;
        ProtobufCodecPtr _codec;
        VirtualHostPtr _host;
        ConsumerManagerPtr _cmp;
        ConnectionManagerPtr _cnmp;
        ThreadPool *_pool;
    public:
        Server(int port, const std::string &basedir)
        : _server(&_baseloop, muduo::net::InetAddress(port), "server", muduo::net::TcpServer::kReusePort),
          _dispatcher(std::bind(&Server::OnUnknownMessage, this, _1, _2, _3)),
          _codec(std::make_shared<ProtobufCodec>(
                  std::bind(&ProtobufDispatcher::onProtobufMessage, &_dispatcher, _1, _2, _3))),
          _host(std::make_shared<VirtualHost>(HOSTNAME, basedir, basedir + DBFILE)),
          _cmp(std::make_shared<ConsumerManager>()),
          _cnmp(std::make_shared<ConnectionManager>()),
          _pool(ThreadPool::getInstance())
        {
            auto map = _host->AllQueue();
            for(auto& it : map)
            {
                _cmp->InitQueueConsumer(it.first);
            }
            _dispatcher.registerMessageCallback<OpenChannelRequest>(std::bind(&Server::OnOpenChannel, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<CloseChannelRequest>(std::bind(&Server::CloseOpenChannel, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<DeclareExchangeRequest>(std::bind(&Server::OnDeclareExchange, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<DeleteExchangeRequest>(std::bind(&Server::OnDeleteExchange, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<DeclareQueueRequest>(std::bind(&Server::OnDeclareQueue, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<DeleteQueueRequest>(std::bind(&Server::OnDeleteQueue, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<QueueBindRequest>(std::bind(&Server::OnQueueBind, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<QueueUnBindRequest>(std::bind(&Server::OnQueueUnBind, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<BasicPublishRequest>(std::bind(&Server::OnBasicPublish, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<BasicAckRequest>(std::bind(&Server::OnBasicAck, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<BasicConsumeRequest>(std::bind(&Server::OnBasicConsume, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<BasicCancelRequest>(std::bind(&Server::OnBasicCancel, this, _1, _2, _3));

            _server.setConnectionCallback(std::bind(&Server::OnConnection, this, _1));
            _server.setMessageCallback(std::bind(&ProtobufCodec::onMessage, _codec.get(), _1, _2, _3));
        }

        void Start()
        {
            _server.start();
            _baseloop.loop();
        }

    private:
        // 辅助函数：获取连接对象并进行检查
        std::shared_ptr<Connection> GetValidConnection(const TcpConnectionPtr& conn, const std::string& action) const
        {
            auto myconn = _cnmp->GetConnection(conn);
            if (!myconn)
            {
                LOG_DEBUG("{}时无法找到连接: {}", action, conn->peerAddress().toIpPort());
                conn->shutdown();
            }
            return myconn;
        }

        // 处理未知报文信息
        void OnUnknownMessage(const TcpConnectionPtr &conn, const MessagePtr &message, muduo::Timestamp) {
            LOG_INFO("未知报文信息 from {}", conn->peerAddress().toIpPort());
        }

        // 处理连接
        void OnConnection(const TcpConnectionPtr &conn) {
            LOG_INFO("{} 连接 {}", conn->peerAddress().toIpPort(), conn->connected() ? "UP" : "DOWN");
        }

        // 打开信道
        void OnOpenChannel(const TcpConnectionPtr &conn, const OpenChannelRequestPtr &req, muduo::Timestamp) {
            auto connection = GetValidConnection(conn, "打开信道");
            if (connection) {
                connection->OpenChannel(req);
            }
        }

        // 关闭信道
        void CloseOpenChannel(const TcpConnectionPtr& conn, const CloseChannelRequestPtr& req, muduo::Timestamp) {
            auto connection = GetValidConnection(conn, "关闭信道");
            if (connection) {
                connection->CloseChannel(req);
            }
        }

        // 定义交换机
        void OnDeclareExchange(const TcpConnectionPtr& conn, const DeclareExchangeRequestPtr& req, muduo::Timestamp) {
            auto connection = GetValidConnection(conn, "定义交换机");
            if (connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if (ch)
                {
                    ch->DeclareExchange(req);
                }
            }
        }

        // 删除交换机
        void OnDeleteExchange(const TcpConnectionPtr& conn, const DeleteExchangeRequestPtr& req, muduo::Timestamp) {
            auto connection = GetValidConnection(conn, "删除交换机");
            if (connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if (ch)
                {
                    ch->DeleteExchange(req);
                }
            }
        }

        // 定义队列
        void OnDeclareQueue(const TcpConnectionPtr& conn, const DeclareQueueRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "定义队列");
            if (connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if (ch)
                {
                    ch->DeclareQueue(req);
                }
            }
        }

        // 删除队列
        void OnDeleteQueue(const TcpConnectionPtr& conn, const DeleteQueueRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "删除队列");
            if (connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if (ch)
                {
                    ch->DeleteQueue(req);
                }
            }
        }

        // 绑定队列
        void OnQueueBind(const TcpConnectionPtr& conn, const QueueBindRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "绑定队列");
            if (connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if (ch)
                {
                    ch->QueueBind(req);
                }
            }
        }

        // 解绑队列
        void OnQueueUnBind(const TcpConnectionPtr& conn, const QueueUnBindRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "解绑队列");
            if (connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if (ch)
                {
                    ch->QueueUnBind(req);
                }
            }
        }

        void OnBasicPublish(const TcpConnectionPtr& conn, const BasicPushlishRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "发布信息");
            if(connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if(ch)
                    ch->BasicPublish(req);
            }
        }
        void OnBasicAck(const TcpConnectionPtr& conn, const BasicAckRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "消息确认");
            if(connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if(ch)  ch->BasicAck(req);
            }
        }
        void OnBasicConsume(const TcpConnectionPtr& conn, const BasicConsumeRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "消息消费");
            if(connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if(ch)
                    ch->BasicConsume(req);
            }
        }

        void OnBasicCancel(const TcpConnectionPtr& conn, const BasicCancelRequestPtr& req, muduo::Timestamp)
        {
            auto connection = GetValidConnection(conn, "信息取消");
            if(connection)
            {
                auto ch = connection->GetChannel(req->cid());
                if(ch)
                    ch->BasicCancel(req);
            }
        }
    };
}