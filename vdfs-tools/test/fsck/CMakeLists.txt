cmake_minimum_required(VERSION 2.6.2)
cmake_policy(SET CMP0015 NEW)

project(test.fsck.vdfs)
add_executable(test.fsck.vdfs test_main.cpp)

target_link_libraries(test.fsck.vdfs gtest_main)

file(GLOB_RECURSE SOURCES *.cpp)
target_sources(test.fsck.vdfs PRIVATE ${SOURCES})

string(REPLACE " " ";" LIB_FILES ${LIB_FILES})
string(REPLACE " " ";" LIB_LIST ${LIB_LIST})
foreach(LIB_FILE ${LIB_FILES})
	target_link_libraries(test.fsck.vdfs ${LIB_FILE})
endforeach()
target_link_libraries(test.fsck.vdfs ${LIB_LIST})

include_directories(../../fsck)
file(GLOB_RECURSE SOURCES ../../fsck/*.o ../../lib/*.o)
list(FILTER SOURCES EXCLUDE REGEX "fsck_main.o")
target_sources(test.fsck.vdfs PRIVATE ${SOURCES})

