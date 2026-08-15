cmake_minimum_required(VERSION 3.16)
project(EDA_SimEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output executable directly to project root folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})


include(FetchContent)

set(CMAKE_TLS_VERIFY OFF)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# --- Source files ---
add_executable(EDA_SimEngine
    src/main.cpp
    src/Netlist.cpp
    src/SimEngine.cpp
    src/Parser.cpp
    src/Tokenizer.cpp
    src/ASTNode.cpp
    src/FileReader.cpp
    src/IOController.cpp
    src/WaveformGUI.cpp
)

# --- Include paths & Linking ---
target_include_directories(EDA_SimEngine PRIVATE include)
target_link_libraries(EDA_SimEngine PRIVATE raylib)

# --- Compiler warnings ---
if (MSVC)
    target_compile_options(EDA_SimEngine PRIVATE /W4)
else()
    target_compile_options(EDA_SimEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
