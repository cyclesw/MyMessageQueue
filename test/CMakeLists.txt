# Define the common source files
set(COMMON_SOURCES
        ../common/msg.pb.h
        ../common/msg.pb.cc
)

# Define the common libraries
set(COMMON_LIBS
        PkgConfig::MyLib
        Boost::boost
        Boost::test_exec_monitor
        Boost::unit_test_framework
)

# Function to create an executable and link common libraries
function(create_executable target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} ${COMMON_LIBS})
endfunction()

# Create executables and link libraries
create_executable(BindingTest bindingTest.cpp)
create_executable(MessageQueueTest messageQueueTest.cpp)
create_executable(MessageTest messageTest.cpp ${COMMON_SOURCES})
create_executable(helperTest helperTest.cpp ${COMMON_SOURCES})
create_executable(routeTest routeTest.cpp)
create_executable(HostTest hostTest.cpp ${COMMON_SOURCES})

# Specific libraries for helperTest


#add_executable(BindingTest bindingTest.cpp
#)

#add_executable(MessageQueueTest messageQueueTest.cpp)

#add_executable(MessageTest messageTest.cpp
#        ../common/msg.pb.h
#        ../common/msg.pb.cc
#)

#add_executable(helperTest helperTest.cpp)

#add_executable(routeTest routeTest.cpp
#)

#add_executable(HostTest hostTest.cpp
#        ../common/msg.pb.h
#        ../common/msg.pb.cc
#)

#target_link_libraries(helperTest PkgConfig::MyLib
#        Boost::boost
#)

#target_link_libraries(HostTest  PkgConfig::MyLib
#        Boost::boost)

#target_link_libraries(MessageTest PkgConfig::MyLib Boost::boost)

#target_link_libraries(routeTest PkgConfig::MyLib Boost::boost)

#target_link_libraries(BindingTest PkgConfig::MyLib )

#target_link_libraries(MessageQueueTest PkgConfig::MyLib)