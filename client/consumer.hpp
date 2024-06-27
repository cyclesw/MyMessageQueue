//
// Created by lang liu on 2024/6/27.
//

#ifndef CLIENT_CONSUMER_HPP
#define CLIENT_CONSUMER_HPP

#include "msg.pb.h"
#include "log.hpp"
#include <unordered_map>
#include <mutex>

namespace MyMQ {
    struct Consumer;

    using ConsumerCallback = std::function<void(const std::string, const BasicProperties*, const std::string)>;
    using ConsumerPtr = std::shared_ptr<Consumer>;

    struct Consumer {
        std::string tag;
        std::string qname;
        bool autoAck{};
        ConsumerCallback callback;

        Consumer() {
            LOG_DEBUG("new Consumer: {}", static_cast<void*>(this));
        }

        Consumer(const std::string& ctag, const std::string& qname, bool auto_ack, ConsumerCallback& cb)
            :tag(ctag), qname(qname), autoAck(auto_ack), callback(std::move(callback)) {}


    };
}

#endif //CLIENT_CONSUMER_HPP
