//
// Created by lang liu on 2024/6/26.
//

#ifndef CLIENT_CHANNEL_HPP
#define CLIENT_CHANNEL_HPP

#include "consumer.hpp"
#include "codec.h"
#include "dispatcher.h"
#include "help.hpp"
#include "mqproto.pb.h"
#include "log.hpp"
#include <memory>
#include <condition_variable>
#include <muduo/net/TcpConnection.h>


namespace MyMQ
{
    class Channel;
    class ChannelManager;

    using ChannelManagerPtr = std::shared_ptr<ChannelManager>;
    using ChannelPtr = std::shared_ptr<Channel>;
    using ProtobufCodecPtr = std::shared_ptr<ProtobufCodec>;
    using BasicCommonResponsePtr = std::shared_ptr<BasicCommonResponse>;
    using BasicConsumeResponsePtr = std::shared_ptr<BasicConsumeResponse>;

    class Channel
    {
    private:
        std::string _cid;
        muduo::net::TcpConnectionPtr _conn;
        ProtobufCodecPtr _codec;
        ConsumerPtr _consumer;
        std::mutex _mutex;
        std::condition_variable _cv;
        std::unordered_map<std::string, BasicCommonResponsePtr> _basicResps;

    private:
        BasicCommonResponsePtr WaitResponse(const std::string& rid) {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [&rid, this] {
               return _basicResps.contains(rid);
            });

            auto ptr = _basicResps[rid];
            _basicResps.erase(rid);

            return ptr;
        }

    public:
        Channel(const muduo::net::TcpConnectionPtr& conn, const ProtobufCodecPtr& codec)
            :_cid(UUIDHelper::UUID()), _conn(conn), _codec(codec) {
        }

        ~Channel() { BasicCancel();}

        std::string cid() { return _cid; }

        bool OpenChannel() {
            OpenChannelRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());

            _codec->send(_conn, req);
            auto resp = WaitResponse(req.rid());
            return resp->ok();
        }

        void CloseChannel() {
            CloseChannelRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            _codec->send(_conn, req);
            WaitResponse(req.rid());
        }

        bool DeclareExchange(const std::string& ename, ExchangeType type,
                bool durable, bool autoDelete, google::protobuf::Map<std::string, std::string>& args) {
            DeclareExchangeRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_durable(durable);
            req.set_auto_delete(autoDelete);
            req.set_exchange_name(ename);
            req.set_exchange_type(type);
            req.mutable_args()->swap(args);
            _codec->send(_conn, req);
            auto resq = WaitResponse(req.rid());
            return resq->ok();
        }

        void DeleteExchange(const std::string& ename) {
            DeleteExchangeRequest req;
            req.set_exchange_name(ename);
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            _codec->send(_conn, req);

            WaitResponse(req.rid());
        }

        bool DeclareQueue(const std::string& qname, bool exclusive, bool durable, bool autoDelete, google::protobuf::Map<std::string, std::string>& args) {
            DeclareQueueRequest req;
            req.set_cid(_cid);
            req.set_queue_name(qname);
            req.set_rid(UUIDHelper::UUID());
            req.set_exclusive(exclusive);
            req.set_durable(durable);
            req.set_auto_delete(autoDelete);
            req.mutable_args()->swap(args);

            _codec->send(_conn, req);
            auto resq = WaitResponse(req.rid());
            return resq->ok();
        }

        void DeleteQueue(const std::string& qname) {
            DeleteQueueRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_queue_name(qname);

            _codec->send(_conn, req);
            WaitResponse(req.rid());
        }


        bool QueueBind(const std::string& ename, const std::string& qname, const std::string& key) {
            QueueBindRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_exchange_name(ename);
            req.set_queue_name(qname);
            req.set_binding_key(key);
            _codec->send(_conn, req);
            auto resq = WaitResponse(req.rid());
            return resq->ok();
        }

        void QueueUnBind(const std::string& ename, const std::string& qname) {
            QueueUnBindRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_exchange_name(ename);
            req.set_queue_name(qname);

            _codec->send(_conn, req);
            WaitResponse(req.rid());
        }

        void BasicPublish(const std::string& ename, const BasicProperties* bp, const std::string& body) {
            BasicPublishRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_exchange_name(ename);
            req.set_body(body);
            if(bp) {
                auto props = req.mutable_properties();
                props->set_id(bp->id());
                props->set_delivery_mode(bp->delivery_mode());
                props->set_routing_key(bp->routing_key());
            }
            _codec->send(_conn, req);
            WaitResponse(req.rid());
        }

        void BasicAck(const std::string& msgid) {
            if (!_consumer) {
                LOG_DEBUG("无消费者");
                return;
            }
            BasicAckRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_message_id(msgid);
            req.set_queue_name(_consumer->qname);

            _codec->send(_conn, req);
             WaitResponse(req.rid());
        }

        bool BasicConsume(const std::string& consumer_tag, const std::string& qname,
                bool autoAck, const ConsumerCallback& cb) {
            if(_consumer.get()) {
                LOG_DEBUG("当前信道已经订阅其他队列信息！");
                return false;
            }
            BasicConsumeRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_consumer_tag(consumer_tag);
            req.set_auto_ack(autoAck);
            req.set_queue_name(qname);
            _codec->send(_conn, req);
            auto resp = WaitResponse(req.rid());
            if(!resp->ok()) {
                return false;
            }
            _consumer = std::make_shared<Consumer>(consumer_tag, qname, autoAck, cb);
            return true;
        }

        void BasicCancel() {
            if (!_consumer.get()) {
                return ;
            }
            BasicCancelRequest req;
            req.set_cid(_cid);
            req.set_rid(UUIDHelper::UUID());
            req.set_queue_name(_consumer->qname);
            req.set_consumer_tag(_consumer->tag);

            _codec->send(_conn, req);
            WaitResponse(req.rid());
            _codec.reset();
        }
    public:
        void PutBasicResponse(const BasicCommonResponsePtr& resp) {
            std::unique_lock<std::mutex> lock(_mutex);
            _basicResps.insert(std::make_pair(resp->rid(), resp));
            _cv.notify_all();
        }

       void Consume(const BasicConsumeResponsePtr& resq) {
            if(!_consumer.get()) {
                LOG_DEBUG("信息处理时，未找到订阅者信息！");
                return;
            }
            if(_consumer->tag != resq->consumer_tag()) {
                LOG_DEBUG("信息处理时，标签不一致");
                return;
            }
            _consumer->callback(resq->consumer_tag(), resq->mutable_properties(), resq->body());
        }
    };

    class ChannelManager {
    private:
        std::mutex _mutex;
        std::unordered_map<std::string, ChannelPtr> _channels;
    public:
        ChannelManager() = default;

        ChannelPtr Create(const muduo::net::TcpConnectionPtr& conn, const ProtobufCodecPtr& codec) {
            std::unique_lock<std::mutex> lock(_mutex);

            auto channel = std::make_shared<Channel>(conn, codec);
            _channels.emplace(channel->cid(), channel);

            return channel;
        }

        void Remove(const std::string& cid) {
            std::unique_lock<std::mutex> lock(_mutex);
            _channels.erase(cid);
        }

        ChannelPtr Get(const std::string& cid) {
            std::unique_lock<std::mutex> lock(_mutex);
            if(!_channels.contains(cid))    return {};

            return _channels[cid];
        }
    };
}


#endif //CLIENT_CHANNEL_HPP
