cmake_minimum_required(VERSION 3.8)
project(SharedMemoryVideo)

add_compile_options(-std=c++17 -Wall -Wextra -Wpedantic -O3)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(sender demo_sender.cpp)
target_link_libraries(sender ${OpenCV_LIBS} ${Boost_LIBRARIES} pthread rt)

add_executable(receiver demo_receiver.cpp)
target_link_libraries(receiver ${OpenCV_LIBS} ${Boost_LIBRARIES} pthread rt)