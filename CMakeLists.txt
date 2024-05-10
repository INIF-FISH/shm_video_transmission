cmake_minimum_required(VERSION 3.0)
project(SharedMemoryImage)

find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(write_to_shared_memory write_to_shared_memory.cpp)
target_link_libraries(write_to_shared_memory ${OpenCV_LIBS} ${Boost_LIBRARIES} pthread rt)

add_executable(read_from_shared_memory read_from_shared_memory.cpp)
target_link_libraries(read_from_shared_memory ${OpenCV_LIBS} ${Boost_LIBRARIES} pthread rt)