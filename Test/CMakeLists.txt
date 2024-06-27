list(APPEND COMMON_LIBS
    PkgConfig::Gtest
)

include_directories(${PROJECT_SOURCE_DIR}/server)

# message("${COMMON_LIBS}")

# Function to create an executable and link common libraries


# Create executables and link libraries
create_executable(BindingTest bindingTest.cpp ${COMMON_SOURCES})
create_executable(MessageQueueTest messageQueueTest.cpp  ${COMMON_SOURCES})
create_executable(MessageTest messageTest.cpp ${COMMON_SOURCES})
create_executable(helperTest helperTest.cpp ${COMMON_SOURCES})
create_executable(routeTest routeTest.cpp  ${COMMON_SOURCES})
create_executable(HostTest hostTest.cpp ${COMMON_SOURCES})
create_executable(ConsumerTest consumerTest.cpp ${COMMON_SOURCES})
create_executable(channelTest channelTest.cpp ${COMMON_SOURCES})

# demo

# add_subdirectory(demo)