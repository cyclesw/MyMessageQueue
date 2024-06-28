//
// Created by lang liu on 2024/6/27.
//

#include "connection.hpp"

using namespace MyMQ;

int main() {
    auto awp = std::make_shared<AsyncWorker>();
    awp->_pool = ThreadPool::getInstance(1);

    auto conn = std::make_shared<Connection>("127.0.0.1", 8888, awp);

    auto ch = conn->OpenChannel();

    google::protobuf::Map<std::string, std::string> gmap;
    ch->DeclareExchange("exchange1", TOPIC, true, true, gmap);

    ch->DeclareQueue("queue1", true, true, true, gmap);

    ch->DeclareQueue("queue2", true, true, true, gmap);

    ch->QueueBind("exchange1", "queue1", "queue1");
    ch->QueueBind("exchange1", "queue2", "news.music.#");

    for(int i = 0; i < 10; ++i) {
        BasicProperties bp;
        bp.set_id(UUIDHelper::UUID());
        bp.set_delivery_mode(DURABLE);
        bp.set_routing_key("news.music.pop");
        ch->BasicPublish("exchange1", &bp, "hello-" + std::to_string(i));
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    BasicProperties bp;
    bp.set_id(UUIDHelper::UUID());
    bp.set_delivery_mode(DURABLE);
    bp.set_routing_key("news.music.pop");
    ch->BasicPublish("exchange1", &bp, "music good bye");

    bp.set_routing_key("news.sport");
    ch->BasicPublish("exchange1", &bp, "Hello sport");

    conn->CloseChannel(ch);

    return 0;
}