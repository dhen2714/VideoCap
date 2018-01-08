cmake_minimum_required(VERSION 3.5)
set (CMAKE_CXX_STANDARD 11)
project(VideoCapture)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)

# Bring the headers into the project.
include_directories(include ${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})

# Add sources with SET command.
set(SOURCES source/main.cpp source/VideoCap.cpp source/CapApp.cpp)

find_package(OpenCV REQUIRED)
add_executable(VideoCapture ${SOURCES})
target_link_libraries(VideoCapture ${OpenCV_LIBS} ${GTKMM_LIBRARIES} -lpthread)
