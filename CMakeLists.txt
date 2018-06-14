cmake_minimum_required(VERSION 3.10)
project(telnet C)

set(CMAKE_C_STANDARD 99)

add_executable(telnet main.c server.h server.c payload.c payload.h peer.h main.h)
add_executable(client_test.c client.c)