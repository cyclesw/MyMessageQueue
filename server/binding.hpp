#pragma once

// #include "msgqueue.hpp"
// #include "exchange.hpp"
#include "help.hpp"


namespace MyMQ
{
    struct Binding;
    class BindingMapper;
    class BindingManager;

    using BindingPtr = std::shared_ptr<Binding>;    //交换机指针
    using MsgQueueBindingMap = std::unordered_map<std::string, BindingPtr>; //交换机队列指针
    using BindingMap = std::unordered_map<std::string, MsgQueueBindingMap>; //
    using BindingManagerPtr = std::shared_ptr<BindingManager>;  

    struct Binding
    {
        std::string exchange_name;
        std::string msgqueue_name;
        std::string binding_key;

        Binding() = default;

        Binding(const std::string& qexchange_name, const std::string& qmsgqueue_name, const std::string& qbinding_key)
            : exchange_name(qexchange_name), msgqueue_name(qmsgqueue_name), binding_key(qbinding_key)
        {}
    };

    class BindingMapper //持久化管理类
    {
    private:
        SqliteHelper _sql_helper;
    public:
        BindingMapper(const std::string& dbfile)
        :_sql_helper(dbfile) 
        {
            std::string path = FileHelper::ParentDirectory(dbfile);
            FileHelper::CreateDirectory(path);
            _sql_helper.Open();
            CreateTable();
        }

        void CreateTable()
        {
            // create table if not exists binding_table(exchange_name varchar(32), msgqueue_name, binding_key)
            std::stringstream sql;
            sql << "create table if not exists binding_table(";
            sql << "exchange_name varchar(32), ";
            sql << "msgqueue_name varchar(32), ";
            sql << "binding_key varchar(128));";
            assert(_sql_helper.Exec(sql.str(), nullptr, nullptr));
        }

        void RemoveTable()
        {
            std::string sql = "drop table if exists binding_table;";
            _sql_helper.Exec(sql, nullptr, nullptr);
        }

        bool Insert(BindingPtr& binding)
        {
            // insert into binding_table values('exchange1', 'msgqueue1', 'news.music.#');
            std::stringstream sql;
            sql << "insert into binding_table values(";
            sql << "'" << binding->exchange_name << "', ";
            sql << "'" << binding->msgqueue_name << "', ";
            sql << "'" << binding->binding_key << "');";

            return _sql_helper.Exec(sql.str(), nullptr, nullptr);
        }

        void Remove(const std::string& ename, const std::string& qname)
        {
            std::stringstream sql;
            sql << "delete from binding_table where ";
            sql << "exchange_name='" << ename << "' and ";
            sql << "msgqueue_name='" << qname << "';";
            _sql_helper.Exec(sql.str(), nullptr, nullptr);
        }

        void RemoveExchangeBindings(const std::string& ename)
        {
            std::stringstream sql;
            sql << "delete from binding_table where ";
            sql << "exchange_name='" << ename << "';";
            _sql_helper.Exec(sql.str(), nullptr, nullptr);
        }

        void RemoveMsgQueueBindings(const std::string& qname)
        {
            std::stringstream sql;
            sql << "delete from binding_table where ";
            sql << "msgqueue_name='" << qname << "';";
            _sql_helper.Exec(sql.str(), nullptr, nullptr);
        }

        BindingMap Recovery()
        {
            BindingMap result;
            std::string sql = "select exchange_name, msgqueue_name, binding_key from binding_table;";
            _sql_helper.Exec(sql, selectCallback, (void*)&result);

            return std::move(result);
        }
    
    private:
        static int selectCallback(void* arg, int numcol, char** row, char** fields)
        {
            BindingMap* result = (BindingMap*)arg;
            BindingPtr bp = std::make_shared<Binding>(row[0], row[1], row[2]);

            MsgQueueBindingMap &qmap = (*result)[bp->exchange_name];
            qmap.insert(std::make_pair(bp->msgqueue_name, bp));

            return 0;
        }
    };

    class BindingManager 
    {
    private:
        std::mutex _mtx;
        BindingMapper _mapper;
        BindingMap _bindings;       
    public:
        explicit BindingManager(const std::string& dbfile)
            :_mapper(dbfile)
        {
            _bindings = _mapper.Recovery();
        }

        bool Bind(const std::string &ename, const std::string &qname, const std::string &key, bool durable) 
        {
            std::unique_lock<std::mutex> lock(_mtx);
            auto it = _bindings.find(ename);
            if(it != _bindings.end() && it->second.find(qname) != it->second.end()) 
                return true;
            
            BindingPtr bp = std::make_shared<Binding>(ename, qname, key);
            if(durable)
            {
                if(!_mapper.Insert(bp))   return false;
            }

            _bindings[ename].insert(std::make_pair(qname, bp));
            return true;
        }

        void UnBind(const std::string &ename, const std::string &qname) 
        {
            auto eit = _bindings.find(ename);
            if(eit == _bindings.end())  return;
            auto qit = eit->second.find(qname);
            if(qit == eit->second.end())    return;

            _mapper.Remove(ename, qname);
            _bindings[ename].erase(qname);
        }

        void RemoveExchangeBindings(const std::string& ename)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _mapper.RemoveExchangeBindings(ename);
            _bindings.erase(ename);
        }

        void RemoveMsgQueueBindings(const std::string& qname)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _mapper.RemoveMsgQueueBindings(qname);
            for(auto & _binding : _bindings)
            {
                _binding.second.erase(qname);
            }

        }

        MsgQueueBindingMap GetExchangeBindings(const std::string &ename) 
        {
            std::unique_lock<std::mutex> lock(_mtx);

            return _bindings[ename];
        }

        BindingPtr GetBinding(const std::string &ename, const std::string &qname) 
        {
            std::unique_lock<std::mutex> lock(_mtx);

            return _bindings[ename][qname];
        }

        bool Exists(const std::string &ename, const std::string &qname) 
        {
            std::unique_lock<std::mutex> lock(_mtx);

            auto eit = _bindings.find(ename);
            if(eit == _bindings.end())  return false;
            auto qit = eit->second.find(qname);

            return qit != eit->second.end();
        }

        size_t Size()
        {
            std::unique_lock<std::mutex> lock(_mtx);
            size_t ret = 0;
            for(auto it = _bindings.begin(); it != _bindings.end(); ++it)
            {
                ret += it->second.size();
            }

            return ret;
        }

        void Clear()
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _mapper.RemoveTable();
            _bindings.clear();
        }
    };
}