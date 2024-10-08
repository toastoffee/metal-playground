cmake_minimum_required(VERSION 3.27)

project(metal_playground)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(metal-cmake)

add_executable( ${PROJECT_NAME}
        main.cpp)

target_link_libraries( ${PROJECT_NAME}
        METAL_CPP
)
