#pragma once

#include "exchange.hpp"
#include "binding.hpp"
#include "message.hpp"
#include "msgqueue.hpp"

namespace rabbitMQ
{
    class VirtualHost;

    using VirtualHostPtr = std::shared_ptr<VirtualHost>;

    class VirtualHost 
    {
    private:
        std::string _hostname;

        ExchangeManagerPtr _emp;
        MsgQueueManagerPtr _mqmp;
        BindingManagerPtr _bmp;
        MessageManagerPtr _mmp;
        
    public:
        VirtualHost(const std::string& hostname, const std::string& basedir, const std::string& dbfile)
        :_hostname(hostname),
        _emp (std::make_shared<ExchangeManager>(dbfile)),
        _mqmp (std::make_shared<MsgQueueManager>(dbfile)),
        _bmp (std::make_shared<BindingManager>(dbfile)),
        _mmp (std::make_shared<MessageManager>(basedir))
        {
            // 恢复历史数据
            auto qm = _mqmp->AllQueues();
            for(auto& it : qm)
            {
                _mmp->InitQueueManager(it.first);
            }
        }

        QueueMap AllQueue()
        {
            return _mqmp->AllQueues();
        }

        void DeleteExchange(const std::string& name)
        {
            _bmp->RemoveExchangeBindings(name);
            _emp->DeleteExchange(name);
        }

        bool DeclareExchange(const std::string& name,
            ExchangeType type, bool durable, bool auto_delete,
            google::protobuf::Map<std::string, std::string>& args)
        {
            return _emp->DeclareExchange(name, type, durable, auto_delete, args);
        }
        
        bool DeclareQueue(const std::string& qname,
            bool qdurable,
            bool qexclusive,
            bool qauto_delete,
            google::protobuf::Map<std::string, std::string>& args)
        {
            _mmp->InitQueueManager(qname);
            return _mqmp->DeclareQueue(qname, qdurable, qexclusive, qauto_delete, args);
        }


        void DeleteQueue(const std::string& qname)
        {
            _mqmp->DeleteQueue(qname);
            _bmp->RemoveMsgQueueBindings(qname);
            _mmp->DestroyQueueMessage(qname);
        }

        bool Bind(const std::string& exchangeName, const std::string& queueName, const std::string& key)
        {
            auto exp = _emp->SelectExchange(exchangeName);
            if(!exp.get())
            {
                LOG_DEBUG("交换机没有队列：{}", exchangeName);
                return false;
            }
            auto mqp = _mqmp->SelectQueue(queueName);
            if(!mqp.get())
            {
                LOG_DEBUG("没有信息队列：{}", queueName);
                return false;
            }
            return _bmp->Bind(exchangeName, queueName, key, exp->durable && mqp->durable);
        }

        void UnBind(const std::string& ExchangeName, const std::string& QueueName)
        {
            auto exp = _emp->SelectExchange(ExchangeName);
            auto mqp = _mqmp->SelectQueue(QueueName);
            _bmp->UnBind(ExchangeName, QueueName);
        }

        MsgQueueBindingMap ExchangeBindings(const std::string& ExchangeName)
        {
            return _bmp->GetExchangeBindings(ExchangeName);
        }

        bool ExistBinding(const std::string& ename, const std::string& qname)
        {
            return _bmp->Exists(ename, qname);
        }

        // 发布信息
        bool BasicPublish(const std::string& qname, BasicProperties* bp,const std::string& body)
        {
            auto mqp = _mqmp->SelectQueue(qname);
            if(!mqp.get())
            {
                LOG_DEBUG("发布信息失败，没有队列:{}", qname);
                return false;
            }
            return _mmp->Insert(qname, bp, body, mqp->durable);
        }

        bool BasicAck(const std::string& qname, const std::string& msgid)
        {
            auto mqp = _mqmp->SelectQueue(qname);
            if(!mqp.get())
            {
                LOG_DEBUG("确定队列信息失败：{}", qname);
                return false;
            }

            return _mmp->Ack(qname, msgid);
        }

        MessagePtr BasicConsume(const std::string& qname)
        {
            return _mmp->Front(qname);
        }

        void Clear()
        {
            _mqmp->Clear();
            _mmp->Clear();
            _bmp->Clear();
            _emp->Clear();
        }

        bool ExistExchange(const std::string& ename)
        {
            return _emp->Exists(ename);
        }

        bool ExistQueue(const std::string & qname)
        {
            return _mqmp->Exists(qname);
        }
    };
};