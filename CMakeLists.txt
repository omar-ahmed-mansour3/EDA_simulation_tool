cmake_minimum_required(VERSION 3.16)
project(EDA_SimEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Source files ---
# Add Parser.cpp and IOController.cpp here as each person finishes their module
add_executable(EDA_SimEngine
    src/main.cpp
    src/Netlist.cpp
    src/SimEngine.cpp
)

# --- Include paths ---
target_include_directories(EDA_SimEngine PRIVATE include)

# --- Compiler warnings ---
if (MSVC)
    target_compile_options(EDA_SimEngine PRIVATE /W4)
else()
    target_compile_options(EDA_SimEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
