cmake_minimum_required(VERSION 3.16)

project(FaceRecognition LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  libfacedetection
  GIT_REPOSITORY https://github.com/ShiqiYu/libfacedetection.git
  GIT_TAG        master
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(DEMO OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libfacedetection)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

add_executable(FaceRecognition
  main.cpp
  mainwindow.cpp
  mainwindow.h
)
target_include_directories(FaceRecognition PRIVATE 
    ${libfacedetection_SOURCE_DIR}/src 
    ${CMAKE_BINARY_DIR}
)
find_package(OpenMP)

target_link_libraries(FaceRecognition PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Gui 
    Qt${QT_VERSION_MAJOR}::Widgets
    facedetection
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(FaceRecognition PRIVATE OpenMP::OpenMP_CXX)
endif()

include(GNUInstallDirs)
install(TARGETS FaceRecognition
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
