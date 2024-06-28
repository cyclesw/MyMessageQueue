//
// Created by lang liu on 2024/6/23.
//

#pragma once

#include "help.hpp"
#include <functional>

namespace MyMQ
{
    struct Consumer;
    class QueueConsumer;
    class ConsumerManager;

    using ConsumerPtr = std::shared_ptr<Consumer>;
    using QueueConsumerPtr = std::shared_ptr<QueueConsumer>;
    using ConsumerManagerPtr = std::shared_ptr<ConsumerManager>;
    using ConsumerCallback = std::function<void(const std::string tag, const BasicProperties* properties, const std::string& msg) >;

    struct Consumer
    {
        std::string tag;
        std::string qname;
        bool autoAck;
        ConsumerCallback callback;

        Consumer() {
            LOG_DEBUG("new Consumer:{}", static_cast<void*>(this));
        }

        ~Consumer() {
            LOG_DEBUG("delete Consumer: {}", static_cast<void*>(this));
        }

        Consumer(const std::string& tag, const std::string& qname, const bool autoAck, const ConsumerCallback& callback)
            :tag(tag), qname(qname), autoAck(autoAck), callback(callback) {
            LOG_DEBUG("new Consumer(args):{}", static_cast<void*>(this));
        }
    };

    class QueueConsumer
    {
    private:
        #define LOCK(mtx) std::unique_lock<std::mutex> lock(mtx)

        std::string _qname;
        std::mutex _mutex;
        uint64_t _rrSeq ;    //轮转号？
        std::vector<ConsumerPtr> _consumers;
    public:
        QueueConsumer() = default;

        explicit QueueConsumer(const std::string& qname) :_qname(qname), _rrSeq(0) {}

        ConsumerPtr Create(const std::string & ctag, const std::string& qname, const bool autoAck, const ConsumerCallback& callback)
        {
            LOCK(_mutex);
            for(auto& it : _consumers)
            {
                if(ctag == it->tag) return {};
            }

            auto consumer = std::make_shared<Consumer>(ctag, qname, autoAck, callback);
            _consumers.push_back(consumer);

            return consumer;
        }

        void Remove(const std::string & ctag)
        {
            LOCK(_mutex);
            for(auto it = _consumers.begin(); it != _consumers.end(); ++it)
            {
                if((*it)->tag == ctag)
                {
                    _consumers.erase(it);
                    break;
                }
            }
        }

        ConsumerPtr Choose()
        {
            LOCK(_mutex);
            if(_consumers.empty())  return {};

            int index = _rrSeq % _consumers.size();
            _rrSeq++;
            // LOG_DEBUG("index:{} _consumers.size:{}", index, _consumers.size());

            return _consumers[index];
        }

        bool Exists(const std::string & tag)
        {
            LOCK(_mutex);
            for(auto& it : _consumers)
            {
                if(it->tag == tag)
                    return true;
            }

            return false;
        }

        bool Empty()
        {
            LOCK(_mutex);
            return true;
        }


        void Clear()
        {
            LOCK(_mutex);
            _consumers.clear();
            _rrSeq = 0;
        }
    };

    class ConsumerManager
    {
    private:
#define LOCK(mtx) std::unique_lock<std::mutex> lock(mtx)

    std::mutex _mutex{};
    std::unordered_map<std::string, QueueConsumerPtr> _qconsumer{};

    private:
        bool findQueue(const std::string& qname, QueueConsumerPtr& qcp)
        {
            auto it = _qconsumer.find(qname);
            if(it == _qconsumer.end())
            {
                LOG_DEBUG("没有队列：{}", qname);
                return false;
            }
            qcp = it->second;
            return true;
        }

    public:
        ConsumerManager() = default;

        void InitQueueConsumer(const std::string& qname)
        {
            LOCK(_mutex);
            auto it = _qconsumer.find(qname);
            if(it != _qconsumer.end())
            {
                LOG_DEBUG("消费者队列已存在:{}", qname);
                return;
            }
            auto qcp = std::make_shared<QueueConsumer>(qname);
            _qconsumer.insert(std::make_pair(qname, qcp));
        }

        ConsumerPtr Create(const std::string& ctag, const std::string& qname, bool ackFlag, const ConsumerCallback& callback)
        {
            QueueConsumerPtr qcp;
            {
                LOCK(_mutex);
                if(!findQueue(qname, qcp)) {
                    return {};
                }
            }

            return qcp->Create(ctag, qname, ackFlag, callback);
        }

        void Remove(const std::string& ctag, const std::string& qname)
        {
            QueueConsumerPtr qcp;
            {
                LOCK(_mutex);
                if(!findQueue(qname, qcp))
                    return;
            }

            return qcp->Remove(ctag);
        }

        void DestoryQueueConsumer(const std::string& qname)
        {
            QueueConsumerPtr qcp;
            {
                LOCK(_mutex);
                if(!findQueue(qname, qcp))
                    return;
                _qconsumer.erase(qname);
            }
            // qcp->Clear();    没有引用自动销毁.
        }

        ConsumerPtr Choose(const std::string& qname)
        {
            QueueConsumerPtr qcp;
            {
                LOCK(_mutex);
                if(!findQueue(qname, qcp)) {
                }
            }

            return qcp->Choose();
        }

        bool Empty()
        {
            LOCK(_mutex);
            return _qconsumer.empty();
        }

        bool Exists(const std::string& ctag, const std::string& qname)
        {
            QueueConsumerPtr qcp;
            {
                LOCK(_mutex);
                if(!findQueue(qname, qcp)) return false;
            }

            return qcp->Exists(ctag);
        }

        void Clear()
        {
            LOCK(_mutex);
            _qconsumer.clear();
        }
    };

}