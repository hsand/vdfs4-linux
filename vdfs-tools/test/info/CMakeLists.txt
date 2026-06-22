cmake_minimum_required(VERSION 2.6.2)

#add_subdirectory(googletest)

project(test.info.vdfs)
add_executable(test.info.vdfs test_main.cpp)

target_link_libraries(test.info.vdfs gtest_main)

file(GLOB_RECURSE SOURCES *.cpp)
target_sources(test.info.vdfs PRIVATE ${SOURCES})

include_directories(../../info)
file(GLOB_RECURSE SOURCES ../../info/*.o)
list(FILTER SOURCES EXCLUDE REGEX "info_main.o")
target_sources(test.info.vdfs PRIVATE ${SOURCES})
target_sources(test.info.vdfs PRIVATE ${SOURCES})

