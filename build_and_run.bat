@echo off
echo ===================================================
echo   Building and Running FaceRecognition Project
echo ===================================================

REM Step 1: Configure the project with CMake
echo.
echo [1/3] Configuring CMake project...
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64"
if %errorlevel% neq 0 (
    echo.
    echo CMake configuration failed!
    pause
    exit /b %errorlevel%
)

REM Step 2: Build the project
echo.
echo [2/3] Compiling and building the executable...
cmake --build build --clean-first --config Release
if %errorlevel% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b %errorlevel%
)

REM Step 3: Run the application
echo.
echo [3/3] Launching FaceRecognition.exe...
start "" ".\build\Release\FaceRecognition.exe"

echo.
echo Done!
