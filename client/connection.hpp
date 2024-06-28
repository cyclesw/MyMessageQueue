//
// Created by lang liu on 2024/6/27.
//

#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP


#include "worker.hpp"
#include "channel.hpp"
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/TcpClient.h>
#include <dispatcher.h>



namespace MyMQ {

    class Connection;

    using ConnectionPtr = std::shared_ptr<Connection>;
    using BasicConsumeRequestPtr = std::shared_ptr<BasicConsumeResponse>;
    using namespace std::placeholders;

    class Connection {
    private:
        muduo::CountDownLatch _latch;
        AsyncWorkerPtr _worker;
        muduo::net::TcpConnectionPtr _conn;
        muduo::net::TcpClient _client;
        ProtobufDispatcher _dispatcher;
        ProtobufCodecPtr _codec;
        ChannelManagerPtr _cmp;

    public:
        Connection(const std::string& ip, int port, const AsyncWorkerPtr& worker)
            :_latch(1), _worker(worker), _client(_worker->_loop.startLoop(), muduo::net::InetAddress(ip, port), "Client"),
            _dispatcher(std::bind(&Connection::OnUnknownMessage, this, _1, _2, _3)),
            _codec(std::make_shared<ProtobufCodec>(std::bind(&ProtobufDispatcher::onProtobufMessage,
                &_dispatcher, _1, _2, _3))),
            _cmp(std::make_shared<ChannelManager>()) {

            _dispatcher.registerMessageCallback<BasicCommonResponse>(std::bind(&Connection::BasicResponse, this, _1, _2, _3));
            _dispatcher.registerMessageCallback<BasicConsumeResponse>(std::bind(&Connection::ConsumeResponse, this, _1, _2, _3));

            _client.setMessageCallback(std::bind(&ProtobufCodec::onMessage, _codec.get(), _1, _2, _3));
            _client.setConnectionCallback(std::bind(&Connection::OnConnection, this, _1));

            _client.connect();
            _latch.wait();
        }

        ChannelPtr OpenChannel() {
            auto channel = _cmp->Create(_conn, _codec);
            if (channel) {
                bool ret = channel->OpenChannel();
                if (!ret) {
                    LOG_DEBUG("信道打开失败");
                    return {};
                }
            }
            return channel;
        }

        void CloseChannel(const ChannelPtr& ptr) {
            ptr->CloseChannel();
            _cmp->Remove(ptr->cid());
        }

    private:
        void BasicResponse(const muduo::net::TcpConnectionPtr& conn, const BasicCommonResponsePtr& resp, muduo::Timestamp) {
            // LOG_DEBUG("进入BasicResponse");
            auto channel = _cmp->Get(resp->cid());
            if(!channel) {
                LOG_DEBUG("信道未找到");
                return;
            }
            channel->PutBasicResponse(resp);
        }

        void ConsumeResponse(const muduo::net::TcpConnectionPtr& conn, const BasicConsumeResponsePtr& resp, muduo::Timestamp) {
            // LOG_DEBUG("进入ConsumeResponse");
            auto channel = _cmp->Get(resp->cid());
            if(!channel) {
                LOG_DEBUG("信道未找到");
                return;
            }
            _worker->_pool->enqueue([channel, resp] {
                channel->Consume(resp);
            });
        }

        void OnUnknownMessage(const muduo::net::TcpConnectionPtr& conn, const MessagePtr& req, muduo::Timestamp) {
            LOG_INFO("UnkownMessage from {}", conn->peerAddress().toIpPort());
        }

        void OnConnection(const muduo::net::TcpConnectionPtr& conn) {
            if(conn->connected()) {
                _latch.countDown();
                _conn = conn;
            }
            else {
                _conn.reset();
            }
            LOG_INFO("Connection {} {}", conn->peerAddress().toIpPort(), conn->connected() ? "UP" : "DOWN");
        }
    };
}

#endif //CLIENTCONNECTION_HPP
