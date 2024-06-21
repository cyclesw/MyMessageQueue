#include "help.hpp"
#include "msg.pb.h"

namespace rabbitMQ {
    #define DATAFILE_SUBFIX ".mqd"
    #define TMPFILE_SUBFIX ".mqd.tmp"

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
        }
    };
}