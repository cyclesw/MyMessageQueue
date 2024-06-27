//
// Created by lang liu on 2024/6/27.
//

#ifndef CLIENT_CONSUMER_HPP
#define CLIENT_CONSUMER_HPP

#include "msg.pb.h"
#include <unordered_map>
#include <mutex>

namespace MyMQ {
    struct ClientConsumer;

    using ClientConsumerCallback = std::function<void(const std::string, const BasicProperties*, const std::string)>;
    using ClientConsumerPtr = std::shared_ptr<ClientConsumer>;

    struct ClientConsumer {
        std::string tag;
        std::string qname;
        bool autoAck;
        ClientConsumerCallback callback;

        ClientConsumer() {
            LOG_DEBUG("new Consumer: {}", this);
        }

        ClientConsumer(const std::string& ctag, const std::string& qname, bool auto_ack, ClientConsumerCallback& cb)
            :tag(ctag), qname(qname), autoAck(auto_ack), callback(std::move(callback)) {}


    };
}

#endif //CLIENT_CONSUMER_HPP
