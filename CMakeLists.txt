cmake_minimum_required(VERSION 3.10.0)
project(vulkan-api CXX)

set(CMAKE_CXX_STANDARD 17)

message(STATUS "Using Vulkan SDK: ${VULKAN_SDK_ROOT} ")

if (WIN32)

elseif (LINUX)
    include_directories(${VULKAN_SDK_ROOT}/x86_64/include)
endif ()

add_subdirectory(src)
