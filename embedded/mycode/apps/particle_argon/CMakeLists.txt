cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(particle_argon)

file(GLOB MYLIB ../../mylib/argon_uart.c)

include_directories(../../include/)

target_sources(app PRIVATE src/main.c ${MYLIB})