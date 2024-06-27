project(client)
include_directories(${PROJECT_SOURCE_DIR}/common)

add_executable(client client.cpp)

target_link_libraries(client ${COMMON_LIBS})