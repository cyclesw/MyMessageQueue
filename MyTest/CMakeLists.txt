# Define the common source files


# Define the common libraries
set(COMMON_LIBS
        spdlog::spdlog
        PkgConfig::MyLib
        Boost::boost
        ${Protobuf_LIBRARIES}
        # Boost::test_exec_monitor
        # Boost::unit_test_framework
)


set(COMMON_SOURCES
        # ../common/msg.pb.h
        # ../common/msg.pb.cc
        # {PROJECT_SOURCE_DIR}/comm/msg.pb.c
        ${PROTO_SRC} 
        ${PROTO_HDRS}
)


# Function to create an executable and link common libraries
function(create_executable target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} ${COMMON_LIBS})
    add_test(NAME ${target} COMMAND ${target})
endfunction()

# Create executables and link libraries
create_executable(BindingTest bindingTest.cpp ${COMMON_SOURCES})
create_executable(MessageQueueTest messageQueueTest.cpp  ${COMMON_SOURCES})
create_executable(MessageTest messageTest.cpp ${COMMON_SOURCES})
create_executable(helperTest helperTest.cpp ${COMMON_SOURCES})
create_executable(routeTest routeTest.cpp  ${COMMON_SOURCES})
create_executable(HostTest hostTest.cpp ${COMMON_SOURCES})
create_executable(ConsumerTest consumerTest.cpp ${COMMON_SOURCES})

# demo

# add_subdirectory(demo)