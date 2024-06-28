project(client)
include_directories(${PROJECT_SOURCE_DIR}/common)

add_executable(client client.cpp
        ${COMMON_SOURCES}
)

add_executable(publishClient publicClient.cpp
        ${COMMON_SOURCES}
)

target_link_libraries(client ${COMMON_LIBS})
target_link_libraries(publishClient ${COMMON_LIBS})