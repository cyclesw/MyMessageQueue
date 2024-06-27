//
// Created by lang liu on 24-4-23.
//

#pragma once

#ifdef DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace MyMQ {
//TODO 初始化日记 完善
    class Log {
    public:
        static Log &getInstance() {
            std::call_once(_flag, []() {
                _instance = new Log();
            });
            return *_instance;
        }

        auto getLogger()
        ->std::shared_ptr<spdlog::logger>
        {
            return _logger;
        }

    private:
        Log() {
            _logger = spdlog::stdout_color_mt("nil");
            _logger->set_level(spdlog::level::debug);
            _logger->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S] [%t] [%s:%#] %v");
        }

        ~Log() {
            spdlog::drop_all();
        }

    private:
        static std::once_flag _flag;
        static Log *_instance;

        std::shared_ptr<spdlog::logger> _logger;
    };

    std::once_flag Log::_flag;
    Log *Log::_instance = nullptr;


#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(Log::getInstance().getLogger(), __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO(Log::getInstance().getLogger(), __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN(Log::getInstance().getLogger(), __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(Log::getInstance().getLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Log::getInstance().getLogger(), __VA_ARGS__)


}

//[x]
// #include <iostream>
// #include <format>
// #include "util.hpp"

// namespace ns_log {

//     using namespace ns_util;

//     enum {
//         INFO,
//         DEBUG,
//         WARN,
//         ERROR,
//         CRITICAL
//     };

//     inline std::ostream &Log(const std::string &level, const std::string &str) {
//         std::string msg = std::format("[{}] [{}] [{}:{}] {}", level, TimeUtil::GetTimeStamp(), __FILE__, __LINE__,
//                                       str);
//         // auto ret = __FILE_NAME__;
//         return std::cout << msg;
//     }

// #define LOG_INFO(...)     Log("INFO", __VA_ARGS__)
// #define LOG_DEBUG(...)    Log("DEBUG", __VA_ARGS__)
// #define LOG_WARN(...)     Log("WARN", __VA_ARGS__)
// #define LOG_ERROR(...)    Log("ERROR", __VA_ARGS__)
// #define LOG_CRITICAL(...) Log("CRITICAL", __VA_ARGS__)

// }
