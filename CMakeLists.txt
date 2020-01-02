cmake_minimum_required(VERSION 3.15)
project(boomer_bot)

include_directories(include)

set(CMAKE_CXX_STANDARD 20)

add_executable(boomer_bot
        src/main.cpp
        include/bot.hpp
        include/utils.hpp
        include/types.hpp
        src/bot.cpp)

target_link_libraries(boomer_bot cpr curl pthread crypto ssl fmt)