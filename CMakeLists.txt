cmake_minimum_required(VERSION 3.15)
project(ImageThresholding)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Point CMake to your MSYS2 Qt installation
set(CMAKE_PREFIX_PATH "C:/msys64/ucrt64")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(ImageThresholding
        frontend/main.cpp
        frontend/mainwindow.cpp
        backend/src/Thresholding.cpp
        backend/src/Segmentation.cpp
)

target_include_directories(ImageThresholding PRIVATE backend/include frontend)
target_link_libraries(ImageThresholding PRIVATE Qt6::Widgets)



# cmake_minimum_required(VERSION 3.15)
# project(ImageThresholding)

# set(CMAKE_CXX_STANDARD 17)

# add_executable(ImageThresholding
#         backend/main.cpp
#         backend/src/Thresholding.cpp
# )

# target_include_directories(ImageThresholding PRIVATE backend/include)
