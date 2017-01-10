
file(GLOB_RECURSE SRC_LIST "${PROJECT_SOURCE_DIR}/test/*.cpp")
set(SRCS ${SRC_LIST})

add_executable(tcp_client_test "tcp_client_test.cpp")
target_link_libraries(tcp_client_test vnet event_pthreads rt)

add_executable(tcp_server_test "tcp_server_test.cpp")
target_link_libraries(tcp_server_test vnet event_pthreads rt)

add_executable(timer_test "timer_test.cpp")
target_link_libraries(timer_test vnet event_pthreads rt)

add_executable(udx_client_test "udx_client_test.cpp")
target_link_libraries(udx_client_test vnet event_pthreads rt)

add_executable(udx_server_test "udx_server_test.cpp")
target_link_libraries(udx_server_test vnet event_pthreads pthread rt)

add_executable(client_test "client_test.cpp")
target_link_libraries(client_test vnet event_pthreads rt)

add_executable(server_test "server_test.cpp")
target_link_libraries(server_test vnet event_pthreads pthread rt)
