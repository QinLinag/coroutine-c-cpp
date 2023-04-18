cmake_minimum_required(VERSION 2.8)
project(coroutine CXX)

set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
    -Wno-unused-parameter
    )

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")


set(SRCS
    coroutine.cc
    main.cc
)

add_executable(Coroutine ${SRCS})

