cmake_minimum_required(VERSION 3.10)
project(http_server LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Boost 库
find_package(Boost 1.82.0 REQUIRED COMPONENTS url system)

find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)


# 包含 Boost 头文件目录
include_directories(${Boost_INCLUDE_DIRS})

# 查找 Gzip 库
find_package(ZLIB REQUIRED)


# 查找源文件
file(GLOB SOURCES "src/*.cpp" "src/*.hpp")

# 添加可执行文件
add_executable(http_server ${SOURCES})

include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(http_server PRIVATE ${OPENSSL_LIBRARIES} Boost::url Boost::system spdlog::spdlog ${ZLIB_LIBRARIES}) 
include_directories(${ZLIB_INCLUDE_DIRS})

# 包含项目源文件目录
target_include_directories(http_server PRIVATE ${CMAKE_SOURCE_DIR}/src)


# 添加编译选项
target_compile_options(http_server PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

# 设置链接选项
set_target_properties(http_server PROPERTIES
        LINK_FLAGS "-pthread"
)


add_dependencies(http_server copy_resources)
add_custom_target(copy_resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/sites
        ${CMAKE_CURRENT_BINARY_DIR}/sites
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/config.ini
        ${CMAKE_CURRENT_BINARY_DIR}/config.ini
)