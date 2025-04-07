cmake_minimum_required(VERSION 3.10)
project(http_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "src/*.cpp" "src/*.hpp")

add_executable(http_server ${SOURCES})

target_include_directories(http_server PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_options(http_server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

set_target_properties(http_server PROPERTIES
    LINK_FLAGS "-pthread"
)