cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 11)
project(VideoCapture)

# Bring the headers into the project.
include_directories(include)

# Add sources with SET command.
set(SOURCES source/main.cpp source/VideoCap.cpp)

find_package(OpenCV REQUIRED)
add_executable(VideoCapture ${SOURCES})
target_link_libraries(VideoCapture ${OpenCV_LIBS})
