cmake_minimum_required(VERSION 3.4...3.28.2 FATAL_ERROR)
project(ttalk)

set(CMAKE_C_STANDARD 23)

add_executable(client src/client.c)
target_link_libraries(client
    PUBLIC ncurses
    PUBLIC pthread
)

add_executable(server src/server.c)
target_link_libraries(server
    PUBLIC pthread
)