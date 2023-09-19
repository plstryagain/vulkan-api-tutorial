add_executable(${PROJECT_NAME} main.cpp)

if (WIN32)

elseif (LINUX)
    target_link_directories(${PROJECT_NAME} PUBLIC "${VULKAN_SDK_ROOT}/x86_64/lib/")
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw vulkan)
endif ()