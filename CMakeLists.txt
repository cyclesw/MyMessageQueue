cmake_minimum_required(VERSION 3.17)
project(MessageQueue)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# if(NOT TARGET spdlog)
#     find_package(spdlog REQUIRED)
# endif()

find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED)
find_package(boost_unit_test_framework REQUIRED)
find_package(boost_test_exec_monitor REQUIRED)

pkg_check_modules(MyLib REQUIRED IMPORTED_TARGET protobuf sqlite3 gtest spdlog)

#set(PROTO_FILES ${PROJECT_SOURCE_DIR}/common/msg.proto)

# build.sh
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ../common/msg.proto)

# set(GLOBAL_LIB 
#     PkgConfig::MyLib
#     Boost::boost
#     spdlog::spdlog
# )

include_directories(${PROJECT_SOURCE_DIR}/common
                ${PROJECT_SOURCE_DIR}/server
                ${PROJECT_SOURCE_DIR}/demo
)       



add_subdirectory(test)


# get_property(ALL_TARGETS GLOBAL PROPERTY ALL_TARGETS)