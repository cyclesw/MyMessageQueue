#pragma once

#include "exchange.hpp"
#include "binding.hpp"
#include "message.hpp"
#include "msgqueue.hpp"

namespace rabbitMQ
{
    class VirtualHost 
    {
    private:
        ExchangeManagerPtr _emp;
        MsgQueueManagerPtr _mqmp;
        BindingManagerPtr _bmp;
        MessageManagerPtr _mmp;
        
    public:
        VirtualHost(const std::string& basedir, const std::string& dbfile);

        void DeleteExchange(const std::string& name);

        bool DeclareExchange(const std::string& name,
            ExchangeType type, bool durable, bool auto_delete,
            std::unordered_map<std::string, std::string>& args);
        
        bool DeclareQueue(const std::string& qname,
            bool qdurable,
            bool qexclusive,
            bool qauto_delete,
            std::unordered_map<std::string, std::string>& args);

        void DeleteQueue(const std::string& qname);

        bool Bind(const std::string& qname, BasicProperties* bp, 
                const std::string& body, DeliveryMode mode);

        void UnBind(const std::string& ename, const std::string& qname);

        MsgQueueBindingMap ExchangeBindings(const std::string& ename);

        bool BasicPublish(const std::string& qname, const std::string& msgid);

        bool BasicAck(const std::string& qname, const std::string& msgid);
    };
};