#pragma once

#include "help.hpp"
#include <memory>
#include <google/protobuf/map.h>

namespace rabbitMQ 
{
    struct Exchange;

    using ExchangePtr = std::shared_ptr<Exchange>;
    using ExchangeMap = std::unordered_map<std::string, ExchangePtr>;

    struct Exchange 
    {

        std::string name;   //交换机名字
        ExchangeType type;  // 交换机类型
        bool durable;   //持久化标志
        bool auto_delete;    //自动删除标志

        google::protobuf::Map<std::string, std::string> args;

        Exchange() = default;

        Exchange(const std::string& ename,
            ExchangeType etype,
            bool edurable,
            bool eauto_delete,
            const google::protobuf::Map<std::string, std::string>& eargs)
            :name(ename), type(etype), durable(edurable),
            auto_delete(eauto_delete), args(eargs) {}

        void SetArgs(const std::string& str_args)
        {
            std::vector<std::string> sub_args;
            StrHelper::Split(sub_args, str_args, "&");
        }                        

        std::string GetArgs()
        {
            std::string result;

            for (auto start = args.begin(); start != args.end(); ++start)
            {
                result += start->first + "=" + start->second + "&";
            }

            return result;
        }

    };

        // 交换机持久化管理类
    class ExchangeMapper 
    {
    public:
        ExchangeMapper(const std::string& dbfile)
        :_sql_helper(dbfile)
        {
            std::string path = FileHelper::ParentDirectory(dbfile);
            FileHelper::CreateDirectory(path);
            assert(_sql_helper.Open());
            CreateTable();
        }

        void CreateTable()
        {   
            #define CREATE_TABLE "create table if not exists exchange_table(\
            name varchar(32) primary key, \
            type int, \
            durable int, \
            auto_delete int, \
            args varchar(128));"

            bool ret = _sql_helper.Exec(CREATE_TABLE, nullptr, nullptr);
            if (ret == false)
            {
                LOG_CRITICAL("创建交换机数据库表失败!!");
                abort();
            }
        }

        void RemoveTable()
        {
            #define DROP_TABLE "drop table if exists exchange_table;"
            bool ret = _sql_helper.Exec(DROP_TABLE, nullptr, nullptr);
            if(ret == false)
            {
                LOG_CRITICAL("删除交换机数据表失败！");
                abort();
            }
        }

        bool Insert(ExchangePtr& exp)
        {
            std::stringstream ss;
            ss << "insert into exchange_table values(";
            ss << "'" << exp->name << "', ";
            ss << exp->type << ", ";
            ss << exp->durable << ", ";
            ss << exp->auto_delete << ", ";
            ss << "'" << exp->GetArgs() << "');";

            return _sql_helper.Exec(ss.str(), nullptr, nullptr);
        }

        void Remove(const std::string& name)
        {
            std::stringstream ss;
            ss << "delete from exchange_table where name=";
            ss << "'" << name << "';";
            _sql_helper.Exec(ss.str(), nullptr, nullptr);
        }

        ExchangeMap Recovery()
        {
            ExchangeMap result;
            std::string sql = "select name, type, durable, auto_delete, args from exchange_table;";
            _sql_helper.Exec(sql, selectCallback, &result);
            return result;  
        }

    private:
        static int selectCallback(void* arg, int numcol, char** row, char** fields)
        {
            ExchangeMap* result = (ExchangeMap*)arg;
            auto exp = std::make_shared<Exchange>();
            exp->name = row[0];
            exp->type = (ExchangeType)std::stoi(row[1]);
            exp->durable = (bool)std::stoi(row[2]);
            exp->auto_delete = (bool)std::stoi(row[3]);
            if (row[4]) exp->SetArgs(row[4]);
            result->insert(std::make_pair(exp->name, exp));
            return 0;
        }

    private:
        SqliteHelper _sql_helper;
    };


    // 交换机数据内存管理类
    class ExchangeManager 
    {
    public:
        ExchangeManager(const std::string& dbfile)
        :_mapper(dbfile)
        {
            _exchanges = _mapper.Recovery();
        }

        bool DeclareExchange(const std::string& name,
            ExchangeType type, bool durable, bool auto_delete,
            const google::protobuf::Map<std::string, std::string>& args)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _exchanges.find(name);
            if(it != _exchanges.end())
                return true;
            
            auto exp = std::make_shared<Exchange>(name, type, durable, auto_delete, args);
            if (durable == true)
            {
                bool ret = _mapper.Insert(exp);
                if(ret == false) return false;
            }

            _exchanges.insert(std::make_pair(name, exp));
            return true;
        }

        void DeleteExchange(const std::string& name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _exchanges.find(name);
            if(it == _exchanges.end())  return;
            
            if(it->second->durable == true) _mapper.Remove(name);
            _exchanges.erase(name);
        }

        ExchangePtr SelectExchange(const std::string& name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _exchanges.find(name);
            if (it == _exchanges.end())
                return ExchangePtr();

            return it->second;
        }

        bool Exists(const std::string& name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _exchanges.find(name);
            if (it == _exchanges.end())
                return false;

            return true;
        }

        size_t Size()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _exchanges.size();
        }

        void Clear()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _mapper.RemoveTable();
            _exchanges.clear();
        }

    private:
        std::mutex _mutex;
        ExchangeMapper _mapper;
        ExchangeMap _exchanges;
    };
}