cmake_minimum_required(VERSION 2.6)

file(GLOB_RECURSE SRC_LIST "${PROJECT_SOURCE_DIR}/src/*.cpp")
set(SRCS ${SRC_LIST})
message(STATUS "---${SRCS}")

add_library(vnet ${SRCS})
target_link_libraries(vnet event FastUdx)

install(TARGETS vnet DESTINATION lib)
