cmake_minimum_required(VERSION 3.16)
project(fake-timers VERSION 1.0.0)

add_library(fake-timers-lib INTERFACE)
target_include_directories(fake-timers-lib INTERFACE include)

add_subdirectory(tests)
