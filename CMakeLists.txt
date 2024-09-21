cmake_minimum_required(VERSION 3.10)

# Project name and version
project(propp VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the include directory as a target
add_library(propp INTERFACE)

target_include_directories(propp
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(propp INTERFACE cxx_std_17)