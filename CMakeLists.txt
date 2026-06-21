cmake_minimum_required(VERSION 2.6)

project(http-server CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "debug")
endif()

set(CXX_FLAGS
 -g
 -Wall
 -Wextra
 -Wno-error=format-truncation
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 -std=c++11
 -rdynamic
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# 查找 muduo 库
find_path(Muduo_INCLUDE_DIR muduo "$ENV{HOME}/muduo")
find_path(Muduo_LIBRARY_DIR libmuduo_net.a "$ENV{HOME}/muduo/build/lib")
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${Muduo_LIBRARY_DIR})
include_directories(${Muduo_INCLUDE_DIR})

find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net)

# Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# 项目源文件
include_directories(${PROJECT_SOURCE_DIR})

add_executable(http-server
  main.cc
  http/HttpResponse.cc
  http/HttpRequest.cc
  http/HttpServer.cc
  handler/StaticFileHandler.cc
)

target_link_libraries(http-server ${muduo_net})
target_link_libraries(http-server ${muduo_base})
target_link_libraries(http-server pthread rt)
