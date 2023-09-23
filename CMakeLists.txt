cmake_minimum_required(VERSION 3.10.0)
project(vulkan-api CXX)

set(CMAKE_CXX_STANDARD 17)

message(STATUS "Using Vulkan SDK: $ENV{VULKAN_SDK} ")
message(STATUS "Using GLM: $ENV{GLM_ROOT} ")
message(STATUS "Using GLFW: $ENV{GLFW_ROOT} ")

if (WIN32)
    include_directories($ENV{VULKAN_SDK}\\Include)
    include_directories($ENV{GLM_ROOT})
    include_directories($ENV{GLFW_ROOT}\\include)
elseif (LINUX)
    include_directories($ENV{VULKAN_SDK}/include)
endif (WIN32)

add_subdirectory(src)
