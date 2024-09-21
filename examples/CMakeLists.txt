cmake_minimum_required(VERSION 3.10)

project(proppexample)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. propp_build)

add_executable(proppexample example.cpp)

add_compile_definitions(propp_ENABLE_MULTITHREADING)

target_link_libraries(proppexample PRIVATE propp)
target_compile_features(proppexample PRIVATE cxx_std_17)
