#include "help.hpp"
#include "msg.pb.h"

namespace rabbitMQ {
    #define DATAFILE_SUBFIX ".mqd"
    #define TMPFILE_SUBFIX ".mqd.tmp"

    class QueueMessage;

    using QueueMessagePtr = std::shared_ptr<QueueMessage>;
    using MessagePtr = std::shared_ptr<Message>;

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
                LOG_ERROR("文件长度与原数据长度不一致");
                return false;
            }

            if(FileHelper(_datafile).Write(body.c_str(), msg->offset(), body.size()) == false)
            {
                LOG_DEBUG("队列写入数据失败");
                return false;
            }

            return true;
        }

        std::list<MessagePtr> gc()
        {
            // 读取有效数据并存入tmp文件，删除datafile
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
                    LOG_ERROR("临时文件写入失败");
                    return result;
                }
            }

            ret = FileHelper::RemoveFile(_datafile);
            ret = FileHelper(_tempfile).rename(_datafile);

            return result;
        }

    private:
        bool insert(const std::string& datafile, MessagePtr& msg)   //TODO 参数一是否有存在意义
        {
            std::string body = msg->payload().SerializeAsString();
            FileHelper helper(datafile);

            size_t fsize = helper.Size();   //文件大小
            size_t msg_size = body.size();

            bool ret = helper.Write((char*)msg_size, fsize, sizeof(size_t));
            if(ret == false)
            {
                LOG_ERROR("写入失败");
                return false;
            }

            ret = helper.Write(body.data(), fsize+sizeof(size_t), body.size());
            if(ret == false)
            {
                LOG_ERROR("写入失败");
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
                    LOG_WARN("读取信息失败!");
                    return false;
                }

                offset += sizeof(size_t);
                std::string msg_body(msg_size, '\0');
                data_file_helper.Read(msg_body.data(), offset, msg_size);
                if(ret == false)
                {
                    LOG_WARN("读取信息失败！");
                    return false;
                }

                offset += msg_size;
                MessagePtr msgq = std::make_shared<Message>();
                msgq->mutable_payload()->ParseFromString(msg_body); //解析字符串
                if(msgq->payload().valid() == "0")
                {
                    LOG_DEBUG("加载到无效消息：{}", msgq->payload().body().c_str());
                    continue;
                }

                result.push_back(msgq);
            }
        }
    };

    class QueueMessage
    {
    private:
        std::list<MessagePtr> _msgs;
        std::unordered_map<std::string, MessagePtr> _durableMsgs;
        std::unordered_map<std::string, MessagePtr> _waitackMsgs;
        MessageMapper _mapper;

        std::mutex _mutex;
        std::string _qname;

        size_t _total_count;
        size_t _valid_count;
    public:
        QueueMessage(std::string& basedir, std::string& qname)
        :_mapper(basedir, qname), _qname(qname), _total_count(0), _valid_count(0)
        {}

        bool Insert(const BasicProperties* properties, std::string& body, bool delivery_mode = true)
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
            std::unique_lock<std::mutex> lock(_mutex);
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
        }

        MessagePtr Front()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto font = _msgs.front();
            _msgs.pop_front();
            _waitackMsgs.insert(std::make_pair(font->payload().properties().id(), font));

            return font;
            // return std::move(font);  c++17后自动优化?
        }

        bool Remove(const std::string& msg_id);

        size_t Clear();

        bool Recovery();

    
    private:
        void GCCheck();

        void gc();
    };
}; 