project(server)

add_executable(server server.cpp
        ${COMMON_SOURCES}
)

target_link_libraries(server ${COMMON_LIBS})