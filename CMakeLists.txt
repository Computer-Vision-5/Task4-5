cmake_minimum_required(VERSION 3.15)
project(ImageThresholding)

set(CMAKE_CXX_STANDARD 17)

add_executable(ImageThresholding
        backend/main.cpp
        backend/src/Thresholding.cpp
)

target_include_directories(ImageThresholding PRIVATE backend/include)
