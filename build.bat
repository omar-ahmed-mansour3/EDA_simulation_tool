@echo off
echo ======================================================================
echo              BUILDING EDA SIMULATION ENGINE (RELEASE)                 
echo ======================================================================
echo.

if not exist build (
    mkdir build
)

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    cd ..
    exit /b %errorlevel%
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    cd ..
    exit /b %errorlevel%
)

cd ..

echo.
echo ======================================================================
echo  BUILD SUCCESSFUL! Executable built.
echo ======================================================================
