cmake_minimum_required(VERSION 2.6.2)

add_subdirectory(googletest)
include_directories(ApprovalTests)

# vdfs-tools include file and definitions
include_directories(../include)
add_definitions(-DUSER_SPACE)
add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-DGTEST_BUILD)

add_subdirectory(info)
add_subdirectory(fsck)

