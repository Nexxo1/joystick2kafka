cmake_minimum_required(VERSION 3.16.3)

project(joystick2kafka)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

# find_package(SimpleITK)

set(SOURCES 
    joystick/joystick.cc
    # joystick/test.cc
    src/main.cpp 
)

include_directories(joystick)

add_executable (${PROJECT_NAME} ${SOURCES})

target_link_libraries ( ${PROJECT_NAME} 
    rdkafka
    )