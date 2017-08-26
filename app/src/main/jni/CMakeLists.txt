cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

add_library(d2cc
            SHARED
            mains.cpp
            d2cc.cpp)

# add lib dependencies
target_link_libraries(d2cc
                      android
                      log
                      )

