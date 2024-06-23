cmake_minimum_required(VERSION 3.20)
project(MessageQueue)

#开启测试
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)

pkg_check_modules(MyLib REQUIRED IMPORTED_TARGET protobuf sqlite3 gtest)

include_directories(${PROJECT_SOURCE_DIR}/common
                ${PROJECT_SOURCE_DIR}/server
                ${PROJECT_SOURCE_DIR}/demo
                ${Protobuf_INCLUDE_DIRS}
                ${CMAKE_CURRENT_BINARY_DIR}
)       


# set(PROTOBUF_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/common)
# FIXME [生成proto file bug?]
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HDRS  ${PROJECT_SOURCE_DIR}/common/msg.proto)

add_executable(ProtoTest MyTest/protoTest.cc ${PROTO_SRC})

target_link_libraries(ProtoTest PkgConfig::MyLib)

add_subdirectory(MyTest)


# get_property(ALL_TARGETS GLOBAL PROPERTY ALL_TARGETS)