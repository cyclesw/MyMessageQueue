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
    using ConsumerCallback = std::function<void(const std::string, const BasicProperties*, const std::string&)>;

    struct Consumer
    {
        std::string tag;
        std::string qname;
        bool autoAck;
        ConsumerCallback cb;

        Consumer() = default;
        Consumer(const std::string& tag, const std::string& qname, const bool autoAck, const ConsumerCallback& callback)
            :tag(tag), qname(qname), autoAck(autoAck), cb(callback)
        {}
    };

    class QueueConsumer
    {
    private:
        #define LOCK(mtx) std::unique_lock<std::mutex> lock(mtx)

        std::string _qname;
        std::mutex _mutex;
        unsigned long long _rrSeq;    //轮转号？
        std::vector<ConsumerPtr> _consumers;
    public:
        QueueConsumer() = default;

        QueueConsumer(const std::string& qname) :_qname(qname) {}

        ConsumerPtr Create(const std::string & ctag, const std::string& cname, const bool autoAck, const ConsumerCallback& callback)
        {
            LOCK(_mutex);
            for(auto& it : _consumers)
            {
                if(ctag == it->tag) return {};
            }

            auto consumer = std::make_shared<Consumer>(ctag, cname, autoAck, callback);
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

            auto index = _rrSeq % _consumers.size();
            _rrSeq++;

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

    std::mutex _mutex;
    std::unordered_map<std::string, QueueConsumerPtr> _qconsumer;

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
            QueueConsumerPtr qcp;
            {
                LOCK(_mutex);
                auto it = _qconsumer.find(qname);
                if(it != _qconsumer.end())
                {
                    LOG_DEBUG("消费者队列已存在:{}", qname);
                    return;
                }
                qcp = std::make_shared<QueueConsumer>();
                _qconsumer.insert(std::make_pair(qname, qcp));
            }
        }

        ConsumerPtr Create(const std::string& ctag, const std::string& qname, bool ackFlag, const ConsumerCallback& callback)
        {
            QueueConsumerPtr qcp;
            {
                LOCK(_mutex);
                if(!findQueue(qname, qcp))
                    return {};
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
                if(!findQueue(qname, qcp))
                    return {};
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