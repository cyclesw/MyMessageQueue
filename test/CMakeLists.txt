add_executable(BindingTest bindingTest.cpp)

add_executable(MessageQueueTest messageQueueTest.cpp)

add_executable(MessageTest messageTest.cpp
        ../common/msg.pb.h
        ../common/msg.pb.cc
)

add_executable(HostTest hostTest.cpp
        ../common/msg.pb.h
        ../common/msg.pb.cc
)

target_link_libraries(HostTest  PkgConfig::MyLib
        Boost::boost)

target_link_libraries(MessageTest PkgConfig::MyLib Boost::boost)


target_link_libraries(BindingTest PkgConfig::MyLib )

target_link_libraries(MessageQueueTest PkgConfig::MyLib)