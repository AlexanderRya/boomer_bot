cmake_minimum_required(VERSION 3.15)
project(dumb_reminder_bot)

include_directories(include)

find_package(cpprestsdk REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_executable(dumb_reminder_bot
        src/main.cpp
        include/bot.hpp
        include/utils.hpp
        include/types.hpp
        src/bot.cpp)

target_link_libraries(dumb_reminder_bot cpr curl pthread cpprestsdk::cpprest fmt)