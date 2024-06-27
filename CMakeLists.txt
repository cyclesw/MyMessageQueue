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

pkg_check_modules(MyLib REQUIRED IMPORTED_TARGET protobuf sqlite3 zlib)
pkg_check_modules(Gtest IMPORTED_TARGET gtest)

message("\n--------------------包查询----------------------------")

message("MYLib:${MyLib_LIBRARIES}")
message("Gtest:${Gtest_LIBRARIES}")

message("----------------------------------------------------\n")

include_directories(${PROJECT_SOURCE_DIR}/common
                ${CMAKE_CURRENT_BINARY_DIR}
)       

file(GLOB PROTO_FILES "${PROJECT_SOURCE_DIR}/common/*.proto")

# file(GLOB PROTO_SRCS "${CMAKE_BINARY_DIR}/*.pb.cc")
set(PROTO_SRCS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_FILE_NAME ${PROTO_FILE} NAME_WE)

    set(PROTO_SRC_FILE ${CMAKE_BINARY_DIR}/${PROTO_FILE_NAME}.pb.cc)
    set(PROTO_HDR_FILE ${CMAKE_BINARY_DIR}/${PROTO_FILE_NAME}.pb.h)

    list(APPEND PROTO_SRCS
        ${PROTO_SRC_FILE}
    )

    if(NOT EXISTS ${PROTO_SRC_FILE} OR NOT EXISTS ${PROTO_HDR_FILE})
        message("执行protoc指令: ${PROTO_FILE_NAME}")
        execute_process(
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                    --cpp_out=${CMAKE_BINARY_DIR}
                    -I ${PROJECT_SOURCE_DIR}/common
                    ${PROTO_FILE}   
            RESULT_VARIABLE PROTOC_RESULT
            ERROR_VARIABLE PROTOC_ERROR
        )
    endif()

    if(${PROTOC_RESULT})
        message(FATAL_ERROR "protoc 执行错误: ${PROTOC_ERROR}")
    endif()
endforeach()



set(COMMON_LIBS
        PkgConfig::MyLib
        spdlog::spdlog
        Boost::boost
        muduo_base
        muduo_net
)


set(COMMON_SOURCES
        ${PROTO_SRCS} 
        ${PROJECT_SOURCE_DIR}/common/codec.cc
)

#message("\n-------------------条件变量测试-----------------------")
#message("Protobuf_PROTOC_EXECUTABLE: ${Protobuf_PROTOC_EXECUTABLE}")
#message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
#message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
#message("Gtest: ${GTEST_LIBRARIES}")
#message("spdlog: ${SPDLOG_LIBRARIES}")
#message("protobuf: ${PROTOBUF_LIBRARIES}")
#message("PROTO_SRCS: ${PROTO_SRCS}")
#message("Sqlite3: ${sqlite3_LIBRARIES}")
#message("command result: ${PROTOC_RESULT}")
#message("comman error:${PROTOC_ERROR}")
#message("----------------------------------------------------\n")

function(create_executable target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} ${COMMON_LIBS})
    add_test(NAME ${target} COMMAND ${target})
endfunction()


add_subdirectory(server)
add_subdirectory(client)

if(Gtest_FOUND)
    add_subdirectory(test)
endif()
# get_property(ALL_TARGETS GLOBAL PROPERTY ALL_TARGETS)

# # FIXME [生成proto file bug?]
# PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HDRS  ${PROJECT_SOURCE_DIR}/common/msg.proto)
# # PROTO_SRC: 只在上方指令执行后设置？
# add_executable(ProtoTest Test/protoTest.cc ${PROTO_SRC})
# # 
# target_link_libraries(ProtoTest PkgConfig::MyLib)