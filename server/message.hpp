#pragma once
#include "help.hpp"
#include "msg.pb.h"


// 消息管理模块
namespace MyMQ {
    #define DATAFILE_SUBFIX ".mqd"
    #define TMPFILE_SUBFIX ".mqd.tmp"

    class QueueMessage;
    class MessageManager;
    class MessageMapper;

    //FIXME MessagePtr 类型不明确
    using QueueMessagePtr = std::shared_ptr<QueueMessage>;
    using MessagePtr = std::shared_ptr<Message>;
    using MessageManagerPtr = std::shared_ptr<MessageManager>;
    using MessageMapperPtr = std::shared_ptr<MessageMapper>;

    class MessageMapper
    {
    private:
        std::string _qname;
        std::string _datafile;
        std::string _tempfile;
    public:
        MessageMapper(std::string& basedir, const std::string& qname)
        {
            if(basedir.back() != '/')
                basedir.push_back('/');

            _datafile = basedir + qname + DATAFILE_SUBFIX;
            _tempfile = basedir + qname + TMPFILE_SUBFIX;
            if(FileHelper(basedir).Exists() == false)
                assert(FileHelper::CreateDirectory(basedir));

            CreateMsgFile();
        }

        bool CreateMsgFile()
        {
            if (FileHelper(_datafile).Exists())
                return true;

            if(!FileHelper::CreateFile(_datafile))
            {
                LOG_ERROR("创建队列数据文件 {} 失败!", _datafile);
                return false;
            }

            return true;
        }

        bool RemoveMsgFile()
        {
            return FileHelper::RemoveFile(_datafile) && FileHelper::RemoveFile(_tempfile);
        }

        bool Insert(MessagePtr& msg)
        {
            return insert(_datafile, msg);
        }

        bool Remove(MessagePtr& msg)
        {   //覆盖原有数据?
            msg->mutable_payload()->set_valid("0"); //设置标志位

            std::string body = msg->payload().SerializeAsString();

            if(body.size() != msg->length())
            {
                LOG_DEBUG("文件长度与原数据长度不一致");
                return false;
            }

            if(FileHelper(_datafile).Write(body.c_str(), msg->offset(), body.size()) == false)  //重写写入原位
            {
                LOG_DEBUG("队列写入数据失败");
                return false;
            }

            return true;
        }

        // 读取有效数据并存入tmp文件，删除datafil
        std::list<MessagePtr> Gc()
        {
            bool ret;
            std::list<MessagePtr> result;
            ret = load(result);
            if(ret == false)
            {
                LOG_ERROR("");
            }

            FileHelper::CreateFile(_tempfile);
            for(auto& it : result)
            {
                LOG_DEBUG("向临时数据写入：{}", it->payload().body());
                ret = insert(_tempfile, it);
                if(!ret)
                {
                    LOG_DEBUG("临时文件写入失败");
                    return result;
                }
            }

            ret = FileHelper::RemoveFile(_datafile);
            ret = FileHelper(_tempfile).rename(_datafile);

            return result;
        }

    private:
        bool insert(const std::string& datafile, MessagePtr& msg)   
        {
            std::string body = msg->payload().SerializeAsString();
            FileHelper helper(datafile);

            size_t fsize = helper.Size();   //文件大小
            size_t msg_size = body.size();

            bool ret = helper.Write((const char*)&msg_size, fsize, sizeof(size_t));
            if(!ret)
            {
                LOG_DEBUG("写入失败");
                return false;
            }

            ret = helper.Write(body.data(), fsize+sizeof(size_t), body.size());
            if(!ret)
            {
                LOG_DEBUG("写入失败");
            }

            msg->set_offset(fsize+sizeof(size_t));
            msg->set_length(body.size());

            return true;
        }

        bool load(std::list<MessagePtr>& result)
        {   //加载所有有效数据
            FileHelper data_file_helper(_datafile);

            size_t offset = 0, msg_size = 0;
            size_t fsize = data_file_helper.Size();

            bool ret;

            while (offset < fsize)
            {
                //将msg_size转成（char*）后读取4字节
                ret = data_file_helper.Read((char*)&msg_size, offset, sizeof(size_t));
                if(ret == false)
                {
                    LOG_DEBUG("读取信息失败!");
                    return false;
                }

                offset += sizeof(size_t);
                std::string msg_body(msg_size, '\0');
                data_file_helper.Read(msg_body.data(), offset, msg_size);
                if(ret == false)
                {
                    LOG_DEBUG("读取信息失败！");
                    return false;
                }

                offset += msg_size;
                MessagePtr msgq = std::make_shared<Message>();
                msgq->mutable_payload()->ParseFromString(msg_body); //解析字符串
                // 返回给gc，gc再返回给insert更新offset、length.
                if(msgq->payload().valid() == "0")
                {
                    LOG_DEBUG("加载到无效消息：{}", msgq->payload().body().c_str());
                    continue;
                }

                result.push_back(msgq);
            }
            
            return true;
        }
    };

    class QueueMessage
    {
    private:
        std::list<MessagePtr> _msgs;    //TODO boost：lookfree?
        std::unordered_map<std::string, MessagePtr> _durableMsgs;
        std::unordered_map<std::string, MessagePtr> _waitackMsgs;
        MessageMapper _mapper;

        std::mutex _mutex;
        std::string _qname;

        size_t _total_count;    
        size_t _valid_count;
        
        #define LOCK(mtx) std::unique_lock<std::mutex> lock(mtx)


    public:
        QueueMessage(std::string& basedir, const std::string& qname)
        :_mapper(basedir, qname), _qname(qname), _total_count(0), _valid_count(0)
        {
            // Recovery();
        }

        bool Insert(const BasicProperties* properties, const std::string& body, bool delivery_mode = true)
        {
            // 构建Msg
            MessagePtr msg = std::make_shared<Message>();
            auto payload = msg->mutable_payload();
            payload->set_body(body);
            if(properties != nullptr)
            {
                payload->mutable_properties()->set_id(properties->id());
                payload->mutable_properties()->set_delivery_mode(properties->delivery_mode());
                payload->mutable_properties()->set_routing_key(properties->routing_key());
            }
            else
            {
                auto mode = delivery_mode ? DeliveryMode::DURABLE : DeliveryMode::UNDURABLE;
                payload->mutable_properties()->set_id(UUIDHelper::UUID());
                payload->mutable_properties()->set_delivery_mode(mode);
            }
            // 判断持久化
            LOCK(_mutex);
            if(payload->properties().delivery_mode() == DeliveryMode::DURABLE)
            {
                msg->mutable_payload()->set_valid("1");
                bool ret = _mapper.Insert(msg);
                if(!ret)
                {
                    LOG_DEBUG("持久化存储信息 {} 失败", body);
                    return false;
                }
                ++_valid_count;
                ++_total_count;
                _durableMsgs.insert(std::make_pair(payload->properties().id(), msg));
            }
            // 加载至内存
            _msgs.push_back(msg);
            return true;
        }

        MessagePtr Front()
        {
            LOCK(_mutex);
            auto font = _msgs.front();
            _msgs.pop_front();
            _waitackMsgs.insert(std::make_pair(font->payload().properties().id(), font));

            return font;
            // return std::move(font);  c++17后自动优化?
        }

        bool Remove(const std::string& msg_id)
        {
            LOCK(_mutex);
            // 寻找信息
            auto it = _waitackMsgs.find(msg_id);
            if(it == _waitackMsgs.end())
            {
                LOG_DEBUG("等待队列寻找信息失败：{}", it->second->payload().body());
                return false;
            }
            auto& payload = it->second->payload();
            // 可持续化删除
            if(payload.properties().delivery_mode() == DeliveryMode::DURABLE)
            {
                _mapper.Remove(it->second);
                _durableMsgs.erase(msg_id);
                --_valid_count;
                Gc();
            }

            //内存删除
            _waitackMsgs.erase(it);
            return true;
        }

        size_t GetTableCount()
        {
            LOCK(_mutex);
            return _msgs.size();
        }

        size_t GetTotalCount()
        {
            LOCK(_mutex);
            return _total_count;
        }

        size_t GetValidCount()
        {
            LOCK(_mutex);
            return _valid_count;
        }

        size_t GetWaitackCount()
        {
            LOCK(_mutex);
            return _waitackMsgs.size();
        }

        size_t GetDurableCount()
        {
            LOCK(_mutex);
            return _durableMsgs.size();
        }

        void Clear()
        {
            LOCK(_mutex);
            _mapper.RemoveMsgFile();
            _waitackMsgs.clear();
            _durableMsgs.clear();
            _msgs.clear();
            _valid_count = _total_count = 0;
        }

        bool Recovery()
        {
            auto msgs = _mapper.Gc();
            for(auto& it : msgs)
            {
                _durableMsgs.insert(std::make_pair(it->payload().properties().id(), it));
            }
            _valid_count = _total_count = msgs.size();
            return true;
        }

    
    private:
        bool GCCheck()
        {
            if(_total_count > 2000 && _valid_count * 10 / _total_count < 5)
                return true;
            else
                return false;
        }

        void Gc()
        {
            if(!GCCheck())  return;
            auto msgs = _mapper.Gc();
            
            for(auto& msg : msgs)
            {
                auto it = _durableMsgs.find(msg->payload().properties().id());
                if(it == _durableMsgs.end())
                {
                    LOG_DEBUG("垃圾回收逻辑错误");
                    _msgs.push_back(msg);
                    _durableMsgs.insert(std::make_pair(msg->payload().properties().id(), msg));
                    continue;
                }

                LOG_DEBUG("MSG->offset:{}", msg->offset());
                LOG_DEBUG("MSG->length:{}", msg->offset());
                // 更新位置
                it->second->set_offset(msg->offset());
                it->second->set_length(msg->length());
            }

            _valid_count = _total_count = msgs.size();
        }
    };

    class MessageManager
    {
    private:
        std::mutex _mutex;
        std::string _basedir;
        std::unordered_map<std::string, QueueMessagePtr> _queMsgs;
        
        #define LOCK(mtx) std::unique_lock<std::mutex> lock(mtx)

    private:
        bool findQueue(const std::string& qname, QueueMessagePtr& qmp) 
        {
            auto it = _queMsgs.find(qname);
            if(it == _queMsgs.end())    
            {
                LOG_DEBUG("没有找到队列:{}", qname);
                return false;
            }

            qmp = it->second;
            return true;
        }

    public:
        explicit MessageManager(const std::string& basedir)
        :_basedir(basedir)
        {}

        void InitQueueManager(const std::string& qname)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto it = _queMsgs.find(qname);
                if(it != _queMsgs.end())    return;
                qmp = std::make_shared<QueueMessage>(_basedir, qname);
                _queMsgs.insert(std::make_pair(qname, qmp));
            }

            qmp->Recovery();
        }

        void DestroyQueueMessage(const std::string& qname)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto it = _queMsgs.find(qname);
                if(it == _queMsgs.end())    return;
                qmp = it->second;
                _queMsgs.erase(it);
            }
            qmp->Clear();
        }

        bool Insert(const std::string& qname, BasicProperties* properties, const std::string& body, bool delivermode= true)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto ret = findQueue(qname, qmp);
                if(!ret)    return false;
            }

            return qmp->Insert(properties, body, delivermode);
        }

        MessagePtr Front(const std::string& qname)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto ret = findQueue(qname, qmp);
                if(!ret)    return MessagePtr();
            }

            return qmp->Front();
        }
        
        // 收到了信息，从等待队列移除
        bool Ack(const std::string qname, const std::string& msg_id)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto ret = findQueue(qname, qmp);
                if(!ret)    return false;
            }

            return qmp->Remove(msg_id);
        }

        void Clear()
        {
            LOCK(_mutex);
            for(auto& it : _queMsgs)
            {
                it.second->Clear();
            }
        }

        size_t GetTableCount(const std::string& qname)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto ret = findQueue(qname, qmp);
                if(!ret)    return -1;
            }
            return qmp->GetTableCount();
        }

        size_t GetTotalCount(const std::string& qname)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto ret = findQueue(qname, qmp);
                if(!ret)    return -1;
            }
            return qmp->GetTotalCount();
        }

        size_t GetDurableCount(const std::string& qname)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto ret = findQueue(qname, qmp);
                if(!ret)    return -1;
            }
            return qmp->GetDurableCount();
        }

        size_t GetWaitackCount(const std::string& qname)
        {
            QueueMessagePtr qmp;
            {
                LOCK(_mutex);
                auto ret = findQueue(qname, qmp);
                if(!ret)    return -1;
            }
            return qmp->GetWaitackCount();
        }
    };
}; 