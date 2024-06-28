//
// Created by lang liu on 2024/6/26.
//


#include "connection.hpp"

using namespace MyMQ;

void cb(ChannelPtr& channel, const std::string tag, const BasicProperties* bp, const std::string& body) {
    std::cout << tag << " 消费了消息：" << body << std::endl;
    channel->BasicAck(bp->id());
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        return 1;
    }

    AsyncWorkerPtr awp = std::make_shared<AsyncWorker>();
    awp->_pool = ThreadPool::getInstance(1);

    ConnectionPtr conn = std::make_shared<Connection>("127.0.0.1", 8888, awp);

    auto ch = conn->OpenChannel();

    google::protobuf::Map<std::string, std::string> gmap;
    ch->DeclareExchange("exchange1", TOPIC, true, true, gmap);

    ch->DeclareQueue("queue1", true, true, true, gmap);
    ch->DeclareQueue("queue2", true, true, true, gmap);
    // ch->DeclareQueue("queue3", true, true, false, gmap);

    ch->QueueBind("exchange1", "queue1", "queue1");
    ch->QueueBind("exchange1", "queue2", "news.music.#");
    // ch->QueueBind("exchange1", "queue3", "queue");

    auto func = std::bind(cb, ch, _1, _2, _3);
    ch->BasicConsume("consumer1", argv[1], false, func);


    while (true)   std::this_thread::sleep_for(std::chrono::seconds(3));
    ch->CloseChannel();

    return 0;
}
