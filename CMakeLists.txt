cmake_minimum_required(VERSION 3.0)
project(SharedMemoryImage)

find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(sender demo_sender.cpp)
target_link_libraries(sender ${OpenCV_LIBS} ${Boost_LIBRARIES} pthread rt)

add_executable(receiver demo_receiver.cpp)
target_link_libraries(receiver ${OpenCV_LIBS} ${Boost_LIBRARIES} pthread rt)