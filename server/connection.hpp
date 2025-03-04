#pragma once

#include "log.hpp"
#include "channel.hpp"

namespace MyMQ
{
    class Connection;
    class ConnectionManager;

    using ConnectionPtr = std::shared_ptr<Connection>;
    using ConnectionManagerPtr = std::shared_ptr<ConnectionManager>;

    class Connection
    {
    private:
        VirtualHostPtr _host;
        ConsumerManagerPtr _cmp;
        muduo::net::TcpConnectionPtr _conn;
        ProtobufCodecPtr _codec;
        ThreadPool* _pool;
        ChannelManagerPtr _channels;
    public:
        Connection(const VirtualHostPtr& host,
                const ConsumerManagerPtr& cmp,
                const muduo::net::TcpConnectionPtr& conn,
                ThreadPool* pool)
                :_host(host), _cmp(cmp), _conn(conn), _pool(pool), _channels(std::make_shared<ChannelManager>())
        {}
        
        void OpenChannel(const OpenChannelRequestPtr& req)
        {
            bool ret = _channels->OpenChannel(req->cid(), _host, _cmp, _codec, _conn, _pool);
            if(!ret)
            {
                LOG_DEBUG("信道ID冲突:{}", req->cid());
                return basicResponse(false, req->rid(), req->cid());
            }
            LOG_DEBUG("信道开启:{}", req->cid());
            return basicResponse(true, req->rid(), req->cid());
        }

        void CloseChannel(const CloseChannelRequestPtr& req)
        {
            _channels->CloseChannel(req->cid());
            return basicResponse(true, req->rid(), req->cid());
        }

        ChannelPtr GetChannel(const std::string& cid)
        {
            return _channels->GetChannel(cid);
        }
    private:
        void basicResponse(bool ok, const std::string& rid, const std::string& cid) const
        {
            BasicCommonResponse resp;
            LOG_DEBUG("basicRespnse to {}", _conn->peerAddress().toIpPort());
            resp.set_rid(rid);
            resp.set_ok(ok);
            resp.set_cid(cid);
            return _codec->send(_conn, resp);
        }
    };

    class ConnectionManager
    {
    private:
        std::mutex _mutex;
        std::unordered_map<muduo::net::TcpConnectionPtr, ConnectionPtr> _conns;
    public:
        ConnectionManager() = default;

        void NewConnection(const VirtualHostPtr& host,
                const ConsumerManagerPtr& cmp,
                const ProtobufCodecPtr& codec,
                const muduo::net::TcpConnectionPtr& conn,
                ThreadPool* pool)  //TODO const？
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _conns.find(conn);
            if(it != _conns.end())
                return;
            auto cp = std::make_shared<Connection>(host, cmp, conn, pool);
            _conns.insert(std::make_pair(conn, cp));
        }
        
        void DeleteConnection(const muduo::net::TcpConnectionPtr& conn)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _conns.erase(conn);
        }

        ConnectionPtr GetConnection(const muduo::net::TcpConnectionPtr& conn)
        {
            return _conns.contains(conn) ? _conns[conn] : ConnectionPtr();
        }
    };
}