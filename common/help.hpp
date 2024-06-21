#pragma once

#include "log.hpp"
#include "msg.pb.h"
#include <sqlite3.h>
#include <boost/algorithm/string.hpp>
#include <google/protobuf/map.h>
#include <random>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

namespace rabbitMQ 
{
    class SqliteHelper 
    {
    public:
        typedef int(*SqliteCallback)(void*,int,char**,char**);


        SqliteHelper(const std::string& dbfile)
        :_dbfile(dbfile), _handler(nullptr)
        {}

        bool Open(int level = SQLITE_OPEN_FULLMUTEX)
        {
            //int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs );
            int ret = sqlite3_open_v2(_dbfile.c_str(), &_handler, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
            if(ret != SQLITE_OK)
            {
                LOG_CRITICAL("数据库打开失败：{}", sqlite3_errmsg(_handler));
                return false;
            }
            return true;
        }

        bool Exec(const std::string &sql, SqliteCallback cb, void *arg) 
        {
            int ret = sqlite3_exec(_handler, sql.c_str(), cb, arg, nullptr);
            if(ret != SQLITE_OK)
            {
                LOG_ERROR("数据库执行失败：{}", sqlite3_errmsg(_handler));
                return false;
            }

            return true;
        }

        void Close()
        {
            if(_handler) sqlite3_close_v2(_handler);
        }

        ~SqliteHelper()
        {
            // close();
        }

    private:
        std::string _dbfile;
        sqlite3 *_handler;
    };

    class StrHelper
    {
    public:
        static void Split(std::vector<std::string>& result, const std::string& str, const std::string& seq)
        {
            boost::split(result, str, boost::is_any_of(seq));
            // size_t pos, idx = 0;
            // while(idx < str.size())
            // {
            //     pos = str.find(seq, idx);
            //     if(pos == std::string::npos)
            //     {
            //         result.emplace_back(str.substr(idx));
            //         return;
            //     }
            //     if(pos == idx)
            //     {   //第一个字母就匹配的情况
            //         idx = pos + seq.size();
            //         continue;
            //     }
            //     result.emplace_back(str.substr(idx, pos - idx));
            //     idx = pos + seq.size();
            // }
        }
    };

    class UUIDHelper 
    {
        static std::string Uuid() {
            std::random_device rd;
            std::mt19937_64 gernator(rd());
            std::uniform_int_distribution<int> distribution(0, 255);
            std::stringstream ss;
            for (int i = 0; i < 8; i++) {
                ss << std::setw(2) << std::setfill('0') << std::hex << distribution(gernator) ;
                if (i == 3 || i == 5 || i == 7) {
                    ss << "-";
                }
            }
            static std::atomic<size_t> seq(1);
            size_t num = seq.fetch_add(1);
            for (int i = 7; i >= 0; i--) {
                ss << std::setw(2) << std::setfill('0') << std::hex << ((num>>(i*8)) & 0xff);
                if (i == 6) ss << "-";
            }
            return ss.str();
        }
    };

    class FileHelper
    {
    public:
        FileHelper(const std::string& filename) :_filename(filename)
        {}

        bool Exists() 
        {
            struct stat st;
            return (stat(_filename.c_str(), &st) == 0);
        }

        size_t Size()
        {
            struct stat st;
            int ret = stat(_filename.c_str(), &st);
            if(ret < 0) return 0;

            return st.st_size;
        }

        bool Read(char* body, size_t offset, size_t len)
        {
            std::ifstream ifs(_filename, std::ios::binary | std::ios::in);
            if (ifs.is_open() == false)
            {
                LOG_ERROR("{} 文件打开失败", _filename);
                return false;
            }

            ifs.seekg(offset, std::ios::beg);
            ifs.read(body, len);
            if(ifs.good() == false)
            {
                LOG_ERROR("{} 文件读取失败", _filename);
                ifs.close();
                return false;
            }

            ifs.close();
            return true;
        }

        bool Write(const char* body, size_t offset, size_t len)
        {
            std::fstream fs(_filename, std::ios::binary | std::ios::in | std::ios::out);
            if (!fs.is_open())
            {
                LOG_ERROR("{} 文件打开失败!", _filename);
                return false;
            }

            fs.seekp(offset, std::ios::beg);
            fs.write(body, len);
            if (!fs.good())
            {
                LOG_ERROR("{} 文件读取失败", _filename);
                fs.close();
                return false;
            }

            fs.close();
            return true;
        }

        bool Write(const std::string& body)
        {
            return Write(body.c_str(), 0, body.size());
        }

        bool rename(const std::string&  nname)
        {
            return (::rename(_filename.c_str(), nname.c_str()) == 0);
        }

        static std::string ParentDirectory(const std::string& filename)
        {
            size_t pos = filename.find_last_of("/");
            if(pos == std::string::npos)
                return "./";

            std::string path = filename.substr(0, pos);
            return path;
        }

        static bool CreateFile(const std::string& filename)
        {
            std::fstream ofs(filename, std::ios::binary | std::ios::out);
            if(!ofs.is_open())
            {
                LOG_ERROR("{} 文件创建失败!", filename.c_str());
                return false;
            }
            ofs.close();
            return true;
        }

        static bool RemoveFile(const std::string& filename)
        {
            return (::remove(filename.c_str()) == 0);
        }

        static bool CreateDirectory(const std::string& path)
        {
            size_t pos, idx = 0;
            while (idx < path.size())
            {
                pos = path.find("/", idx);
                if (pos == std::string::npos)
                    return mkdir(path.c_str(), 0775) == 0;
                
                std::string subpath = path.substr(0, pos);
                int ret = mkdir(subpath.c_str(), 0775);
                if ( ret != 0 & errno != EEXIST)
                {
                    LOG_ERROR("创建目录 {} 失败: {}", subpath, strerror(errno));
                    return false;
                }

                idx = pos + 1;
            }
            return true;
        }

        static bool RemoveDirectory(const std::string& path)
        {
            std::string cmd = "rm -rf " + path;
            return (system(cmd.c_str()) != -1);
        }

    private:
        std::string _filename;
    };
};