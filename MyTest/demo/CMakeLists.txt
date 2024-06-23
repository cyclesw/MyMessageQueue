set(MUDUO_LIB muduo_net
    muduo_base
)

# createFILE
create_executable(EchoTest muduo/echo/server.cc)
create_executable(MPServerTest muduo/proto/server.cpp
    muduo/proto/request.pb.cc
)
create_executable(MPClient muduo/proto/client.cpp
)

target_link_libraries(EchoTest  MUDUO_LIB)

target_link_libraries(EchoTest MUDUO_LIB)

target_link_libraries(MPClient MUDUO_LIB)