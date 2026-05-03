cmake_minimum_required(VERSION 3.15)
project(ImageThresholding)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# NOTE (Windows): avoid hard-coding a Qt path here.
# Configure Qt via your CLion CMake Profile, e.g.:
#   -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/mingw_64
# or by setting Qt6_DIR.
# (Hard-coding often leads to toolchain/Qt mismatches and missing DLLs at runtime.)
# set(CMAKE_PREFIX_PATH "C:/msys64/ucrt64")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(ImageThresholding
        frontend/main.cpp
        frontend/mainwindow.cpp
        backend/src/Thresholding.cpp
        backend/src/Segmentation.cpp
)

target_include_directories(ImageThresholding PRIVATE backend/include frontend)
target_link_libraries(ImageThresholding PRIVATE Qt6::Widgets)

# Fix for: Process finished with exit code -1073741515 (0xC0000135)
# 0xC0000135 usually means a required DLL can't be found at startup.
# On Windows, deploy Qt + compiler runtime DLLs next to the built executable.
if(WIN32)
        # Qt's CMake packages typically populate this cache entry.
        if(NOT WINDEPLOYQT_EXECUTABLE)
                # Try to locate windeployqt relative to Qt6::qmake.
                get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)
                if(_qt6_qmake_location)
                        get_filename_component(_qt6_bin_dir "${_qt6_qmake_location}" DIRECTORY)
                        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt6_bin_dir}")
                endif()
        endif()

        if(WINDEPLOYQT_EXECUTABLE)
                add_custom_command(
                        TARGET ImageThresholding
                        POST_BUILD
                        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                                --dir "$<TARGET_FILE_DIR:ImageThresholding>"
                                --compiler-runtime
                                --no-translations
                                "$<TARGET_FILE:ImageThresholding>"
                        COMMENT "Deploying Qt runtime (windeployqt) to output directory"
                        VERBATIM
                )
        else()
                message(WARNING "windeployqt.exe not found; the app may fail to start unless Qt/bin is on PATH.")
        endif()
endif()



# cmake_minimum_required(VERSION 3.15)
# project(ImageThresholding)

# set(CMAKE_CXX_STANDARD 17)

# add_executable(ImageThresholding
#         backend/main.cpp
#         backend/src/Thresholding.cpp
# )

# target_include_directories(ImageThresholding PRIVATE backend/include)
