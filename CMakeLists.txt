cmake_minimum_required(VERSION 3.16)
project(EDA_SimEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

# --- Include paths ---
target_include_directories(EDA_SimEngine PRIVATE include)

# --- Compiler warnings ---
if (MSVC)
    target_compile_options(EDA_SimEngine PRIVATE /W4)
else()
    target_compile_options(EDA_SimEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
