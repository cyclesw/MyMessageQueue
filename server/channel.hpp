#pragma once

#include "threadpool.hpp"
#include "route.hpp"
#include "consumer.hpp"
#include "mqproto.pb.h"
#include "host.hpp"
#include "codec.h"
#include "dispatcher.h"
#include "help.hpp"

namespace MyMQ
{
    using namespace std::placeholders;

    class Channel;
    class ChannelManager;

    using ChannelPtr = std::shared_ptr<Channel>;
    using ChannelManagerPtr = std::shared_ptr<ChannelManager>;
    using ProtobufCodecPtr = std::shared_ptr<ProtobufCodec>;

    using OpenChannelRequestPtr = std::shared_ptr<OpenChannelRequest>;
    using CloseChannelRequestPtr = std::shared_ptr<CloseChannelRequest>;

    using DeclareExchangeRequestPtr = std::shared_ptr<DeclareExchangeRequest>;
    using DeclareQueueRequestPtr = std::shared_ptr<DeclareQueueRequest>;
    using DeleteExchangeRequestPtr = std::shared_ptr<DeleteExchangeRequest>;
    using DeleteQueueRequestPtr = std::shared_ptr<DeleteQueueRequest>;

    using QueueBindRequestPtr = std::shared_ptr<QueueBindRequest>;
    using QueueUnBindRequestPtr = std::shared_ptr<QueueUnBindRequest>;

    using BasicPushlishRequestPtr = std::shared_ptr<BasicPushlishRequest>;
    using BasicAckRequestPtr = std::shared_ptr<BasicAckRequest>;
    using BasicConsumeRequestPtr = std::shared_ptr<BasicConsumeRequest>;
    using BasicConsumeResponsePtr = std::shared_ptr<BasicConsumeResponse>;
    using BasicCancelRequestPtr = std::shared_ptr<BasicCancelRequest>;
    using BasicCommonResponsePtr = std::shared_ptr<BasicCommonResponse>;

    class Channel
    {
    private:
        std::string _cid;
        ConsumerManagerPtr _cmp;
        ConsumerPtr _consumer;
        muduo::net::TcpConnectionPtr _conn;
        VirtualHostPtr _host;
        ProtobufCodecPtr _codec;
        ThreadPool* _pool;
    public:
        Channel(const std::string& id, const VirtualHostPtr& host, const ConsumerManagerPtr& cmp,
                const ProtobufCodecPtr& codec, const muduo::net::TcpConnectionPtr& conn, ThreadPool* pool)
                :_cid(id), _host(host), _cmp(cmp), _codec(codec), _conn(conn), _pool(pool)
        {
            LOG_DEBUG("new channel");
        }

        ~Channel()
        {
            if(_cmp.get() != nullptr)
            {
                _cmp->Remove(_consumer->tag, _consumer->qname);
            }
            LOG_DEBUG("delete Channel");
        }

        void DeclareExchange(const DeclareExchangeRequestPtr& rep)
        {
            bool ret = _host->DeclareExchange(rep->exchangename(), rep->exchangetype(), rep->durable(), rep->autodelete(), rep->args());
            return basicResponse(ret, rep->rid(), rep->cid());
        }

        void DeleteExchange(const DeleteExchangeRequestPtr& req)
        {
            _host->DeleteExchange(req->exchangename());
            return basicResponse(true, req->rid(), req->cid());
        }

        void DeclareQueue(const DeclareQueueRequestPtr& req)
        {
            bool ret = _host->DeclareQueue(req->queuename(), req->durable(), req->exclusive(), req->autodelete(), req->args());

            if(!ret)
                return basicResponse(false, req->rid(), req->cid());
            
            _cmp->InitQueueConsumer(req->queuename());
            return basicResponse(true, req->rid(), req->cid());
        }

        void DeleteQueue(const DeleteQueueRequestPtr& req)
        {
            _host->DeleteQueue(req->queuename());
            _cmp->DestoryQueueConsumer(req->queuename());
            return basicResponse(true, req->rid(), req->cid());
        }

        void QueueBind(const QueueBindRequestPtr& req)
        {
            bool ret = _host->Bind(req->exchangename(), req->queuename(), req->bindingkey());
            return basicResponse(ret, req->rid(), req->cid());
        }

        void QueueUnBind(const QueueUnBindRequestPtr& req)
        {
            _host->UnBind(req->exchangename(), req->queuename());
            return basicResponse(true, req->rid(), req->cid());
        }

        void BasicPublish(const BasicPushlishRequestPtr& req)
        {
            // 选择交换机
            auto exp = _host->SelectExchange(req->exchangename());
            if(!exp.get())  return basicResponse(false, req->rid(), req->cid());

            // 获取交换机中的绑定队列
            auto map = _host->ExchangeBindings(req->exchangename());
            BasicProperties* bp;
            std::string routingKey;
            if(req->has_properties())
            {
                bp = req->mutable_properties();
                routingKey = bp->routing_key();
            }
            // 推送信息
            for(auto& it : map)
            {
                //  路由匹配则发送
                if(Router::Route(exp->type, bp->routing_key(), it.second->binding_key))
                {
                    _host->BasicPublish(it.first, bp, req->body());
                    auto task = std::bind(&Channel::consume, this, it.first);
                    _pool->enqueue(task);
                }
            }
            return basicResponse(true, req->rid(), req->cid());
        }

        void BasicConsume(const BasicConsumeRequestPtr& req)
        {
            bool ret = _host->ExistQueue(req->queuename());
            if(!ret)    return basicResponse(false, req->rid(), req->cid());
            auto cb = std::bind(&Channel::callback, this, _1, _2, _3);
            _consumer = _cmp->Create(req->consumertag(), req->queuename(), req->autoack(), cb);

            return basicResponse(true, req->rid(), req->cid());
        }

        void BasicCancel(const BasicCancelRequestPtr& req)
        {
            _cmp->Remove(req->consumertag(), req->queuename());
            return basicResponse(true, req->rid(), req->cid());
        }

        void BasicAck(const BasicAckRequestPtr& req)
        {
        }

    private:
        void callback(const std::string& tag, const BasicProperties* bp, const std::string& body)
        {
            BasicConsumeResponse resp;
            resp.set_cid(_cid);
            resp.set_body(body);
            resp.set_consumertag(tag);
            if(bp)
            { 
                resp.mutable_properties()->set_id(bp->id());
                resp.mutable_properties()->set_delivery_mode(bp->delivery_mode());
                resp.mutable_properties()->set_routing_key(bp->routing_key());
            }
            
            _codec->send(_conn, resp);
        }

        void consume(const std::string& qname)
        {
            auto mp = _host->BasicConsume(qname);
            if(!mp.get())
            {
                LOG_DEBUG("消费任务执行失败：{} 没有信息");
                return;
            }
            auto cp = _cmp->Choose(qname);
            if(!cp.get())
            {
                LOG_DEBUG("消费任务执行失败：{} 队列没有消费者", qname);
                return;
            }
            cp->callback(cp->tag, mp->mutable_payload()->mutable_properties(), mp->payload().body());
            if(cp->autoAck)
            {
                _host->BasicAck(qname, mp->payload().properties().id());
            }
        }

        void basicResponse(bool Ok, const std::string& rid, const std::string& cid)
        {
            BasicCommonResponse resp;
            resp.set_rid(rid);
            resp.set_cid(cid);
            resp.set_ok(Ok);
            _codec->send(_conn, resp);
        }
    };

    class ChannelManager
    {
    private:
        std::mutex _mutex;
        std::unordered_map<std::string, ChannelPtr> _channels;
    public:
        ChannelManager() = default;

        bool OpenChannel(const std::string& cid,
                const VirtualHostPtr& host,
                const ConsumerManagerPtr& cmp,
                const ProtobufCodecPtr& codec,
                const muduo::net::TcpConnectionPtr& conn,
                ThreadPool* pool)
        {
            std::unique_lock<std::mutex> lock(_mutex);

            auto it = _channels.find(cid);
            if(it != _channels.end())
            {
                LOG_DEBUG("信道 {} 已经存在", cid);
                return false;
            }
            auto channel = std::make_shared<Channel>(cid, host, cmp, codec, conn, pool);
            _channels.insert(std::make_pair(cid, channel));
            return true;
        }

        void CloseChannel(const std::string& cid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _channels.erase(cid);
        }

        ChannelPtr GetChannel(const std::string& cid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _channels.find(cid)->second;
        }
    };
}