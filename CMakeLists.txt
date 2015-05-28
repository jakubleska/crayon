cmake_minimum_required(VERSION 2.6)
project(colorizer)

SET(CMAKE_CXX_FLAGS "-std=c++11 -Wall -pedantic")

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.45.0 COMPONENTS program_options) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    add_executable(colorizer main.cc)
    target_link_libraries(colorizer ${Boost_LIBRARIES})
endif()





install(TARGETS colorizer RUNTIME DESTINATION bin)
