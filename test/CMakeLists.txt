add_executable(Test test.cpp)

add_executable(BindingTest bindingTest.cpp)

add_executable(MessageTest messageTest.cpp)

target_link_libraries(MessageTest PkgConfig::MyLib)

target_link_libraries(Test PkgConfig::MyLib)

target_link_libraries(BindingTest PkgConfig::MyLib )