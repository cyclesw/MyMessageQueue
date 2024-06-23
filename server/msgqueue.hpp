#pragma once

#include "help.hpp"

// 队列数据管理模块
namespace MyMQ
{
    struct MsgQueue;
    class MsgQueueManager;

    using MsgQueueManagerPtr = std::shared_ptr<MsgQueueManager>;
    using MsgQueuePtr = std::shared_ptr<MsgQueue>;
    using QueueMap = std::unordered_map<std::string, MsgQueuePtr>;

    struct MsgQueue 
    {
        MsgQueue() = default;

        MsgQueue(const std::string& qname, 
            bool qdurable, 
            bool qexclusive, 
            bool qautoDelete, 
            const google::protobuf::Map<std::string, std::string>& qargs)
            : name(qname),
              durable(qdurable),
              exclusive(qexclusive),
              auto_delete(qautoDelete),
              args(qargs)
        {}

        void SetArgs(const std::string& str_args)
        {
            std::vector<std::string> sub_args;
            StrHelper::Split(sub_args, str_args, "&");
            for (auto& str: sub_args)
            {
                size_t pos = str.find("=");
                std::string key = str.substr(0, pos);
                std::string value = str.substr(pos + 1);
                args[key] = value;
            }
        }

        std::string GetArgs()
        {
            std::string result;
            for(auto start = args.begin(); start != args.end(); ++start)
            {
                result += start->first + "=" + start->second + "&";
            }

            return result;
        }

        std::string name;
        bool durable;
        bool exclusive;
        bool auto_delete;
        google::protobuf::Map<std::string, std::string> args;
    };

    class MsgQueueMapper
    {
    private:
        SqliteHelper _sql_helper;
    public:
        ~MsgQueueMapper() = default;

        MsgQueueMapper(const std::string& dbfile)
        :_sql_helper(dbfile)
        {
            std::string path = FileHelper::ParentDirectory(dbfile);
            FileHelper::CreateDirectory(path);
            _sql_helper.Open();
            CreateTable();
        }

        void CreateTable()
        {
            std::stringstream sql;
            sql << "create table if not exists queue_table(";
            sql << "name varchar(32) primary key, ";
            sql << "durable int, ";
            sql << "exclusive int, ";
            sql << "auto_delete int, ";
            sql << "args varchar(128));";
            assert(_sql_helper.Exec(sql.str(), nullptr, nullptr));
        }

        void RemoveTable()
        {
            std::stringstream sql;
            sql << "drop table if exists queue_table;";
            _sql_helper.Exec(sql.str(), nullptr, nullptr);
        }

        bool Insert(MsgQueuePtr& queue)
        {
            std::stringstream sql;
            sql << "insert into queue_table values(";
            sql << "'" << queue->name << "', ";
            sql << queue->durable << ", ";
            sql << queue->exclusive << ", ";
            sql << queue->auto_delete << ", ";
            sql << "'" << queue->GetArgs() << "');";
            return _sql_helper.Exec(sql.str(), nullptr, nullptr);
        }

        void Remove(const std::string& name)
        {
            std::stringstream sql;
            sql << "delete from queue_table where name=";
            sql << "'" << name << "';";
            _sql_helper.Exec(sql.str(), nullptr, nullptr);
        }

        QueueMap Recovery()
        {
            QueueMap result;
            std::string sql = "select name, durable, exclusive, auto_delete, args from queue_table;";
            assert(_sql_helper.Exec(sql, selectCallback, &result));
            return result;
        }

    private:
        static int selectCallback(void* data, int argc, char** argv, char** azColName)
        {
            QueueMap* result = static_cast<QueueMap*>(data);
            MsgQueuePtr queue(new MsgQueue());
            queue->name = argv[0];
            queue->durable = (bool)atoi(argv[1]);
            queue->exclusive = (bool)atoi(argv[2]);
            queue->auto_delete = (bool)atoi(argv[3]);
            if(argv[4]) queue->SetArgs(argv[4]);
            result->insert
            (std::make_pair(queue->name, queue));
            return 0;
        }
    };
    

    class MsgQueueManager
    {
    private:
        std::mutex _mutex;
        MsgQueueMapper _mapper;
        QueueMap _msg_queues;
    public:
        MsgQueueManager(const std::string &dbfile):_mapper(dbfile) 
        {
            _msg_queues = _mapper.Recovery();
        }

        bool DeclareQueue(const std::string &qname, 
            bool qdurable, 
            bool qexclusive,
            bool qauto_delete,
            const google::protobuf::Map<std::string, std::string> &qargs) {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _msg_queues.find(qname);
            if (it != _msg_queues.end()) {
                return true;
            }
            MsgQueuePtr mqp = std::make_shared<MsgQueue>();
            mqp->name = qname;
            mqp->durable = qdurable;
            mqp->exclusive = qexclusive;
            mqp->auto_delete = qauto_delete;
            mqp->args = qargs;
            if (qdurable == true) 
            {
                bool ret = _mapper.Insert(mqp);
                if (ret == false) return false;
            }
            _msg_queues.insert(std::make_pair(qname, mqp));
            return true;
        } 

        void DeleteQueue(const std::string &name) {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _msg_queues.find(name);
            if (it == _msg_queues.end()) {
                return ;
            }
            if (it->second->durable == true) _mapper.Remove(name);
            _msg_queues.erase(name);
        }

        MsgQueuePtr SelectQueue(const std::string &name) 
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _msg_queues.find(name);
            if (it == _msg_queues.end()) 
            {
                return MsgQueuePtr();
            }
            return it->second;
        }

        QueueMap AllQueues() {
            std::unique_lock<std::mutex> lock(_mutex);
            return _msg_queues;
        }

        bool Exists(const std::string &name) 
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _msg_queues.find(name);
            if (it == _msg_queues.end()) 
            {
                return false;
            }
            return true;
        }

        size_t Size()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _msg_queues.size();
        }

        void Clear() 
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _msg_queues.clear();
            _mapper.RemoveTable();
        }
    };
} // namespace rabbitMq

