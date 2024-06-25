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
    #define DBFILE "/meta.db"
    #define HOSTNAME "MyVHost"

    using namespace muduo::net;

    using MessagePtr = std::shared_ptr<google::protobuf::Message>;

    class Server
    {
    private:
        EventLoop _baseloop;
        TcpServer _server;
        ProtobufDispatcher _dispatcher;
        ProtobufCodecPtr _codec;
        VirtualHostPtr _host;
        ConsumerManagerPtr _cmp;
        ConnectionMannagerPtr _cnmp;
        ThreadPool* _pool;
    public:
        Server(int port, const std::string& basedir)
        :_server(&_baseloop, muduo::net::InetAddress(port), "server", muduo::net::TcpServer::kReusePort),
         _dispatcher(std::bind(&Server::OnUnknownMessage, this, _1, _2, _3)),
         _codec(std::make_shared<ProtobufCodec>(std::bind(&ProtobufDispatcher::onProtobufMessage, &_dispatcher, _1, _2, _3))),
         _host(std::make_shared<VirtualHost>(HOSTNAME, basedir, DBFILE)),
         _cmp(std::make_shared<ConsumerManager>()),
         _cnmp(std::make_shared<ConnectionMannager>()),
         _pool(ThreadPool::getInstance())
        {
            auto map = _host->AllQueue();
            for(auto& it : map) {
            }
        }
        
    private:
        void OnUnknownMessage(const TcpConnectionPtr& conn, const MessagePtr& message, muduo::Timestamp);

        void OnConnection(const TcpConnectionPtr& conn);

        void OnOpenChannel(const TcpConnectionPtr& conn, const OpenChannelRequestPtr& req, muduo::Timestamp);

        void CloseOpenChannel(const TcpConnectionPtr& conn, const CloseChannelRequestPtr& req, muduo::Timestamp);

        void OnDeclareExchange(const TcpConnectionPtr& conn, const DeclareExchangeRequestPtr& req, muduo::Timestamp);

        void OnDeleteExchange(const TcpConnectionPtr& conn, const DeleteExchangeRequestPtr& req, muduo::Timestamp);

        void OnDeclareQueue(const TcpConnectionPtr& conn, const DeclareQueueRequestPtr& req, muduo::Timestamp);

        void OnDeleteQueue(const TcpConnectionPtr& conn, const DeleteQueueRequestPtr& req, muduo::Timestamp);

        void OnQueueBind(const TcpConnectionPtr& conn, const QueueBindRequestPtr& req, muduo::Timestamp);

        void OnQueueUnBind(const TcpConnectionPtr& conn, const QueueUnBindRequestPtr& req, muduo::Timestamp);

        void OnBasicPublish(const TcpConnectionPtr& conn, const BasicPushlishRequestPtr& req, muduo::Timestamp);

        void OnBasicAck(const TcpConnectionPtr& conn, const BasicAckRequestPtr& req, muduo::Timestamp);

        void OnBasicConsume(const TcpConnectionPtr& conn, const BasicConsumeRequestPtr& req, muduo::Timestamp);

        void OnBasicCannel(const TcpConnectionPtr& conn,const BasicCancelRequestPtr& req, muduo::Timestamp);
    };
}