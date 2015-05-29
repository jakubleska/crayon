cmake_minimum_required(VERSION 2.6)
project(crayon)

SET(CMAKE_CXX_FLAGS "-std=c++11 -Wall -pedantic")

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.45.0 COMPONENTS program_options) 
find_package (Threads REQUIRED)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
endif()


add_executable(colorizer main.cc)


if(Boost_FOUND)
    target_link_libraries(colorizer ${Boost_LIBRARIES})
endif()

if(Threads_FOUND)
    target_link_libraries(colorizer ${CMAKE_THREAD_LIBS_INIT})
endif()


install(TARGETS colorizer RUNTIME DESTINATION bin)
