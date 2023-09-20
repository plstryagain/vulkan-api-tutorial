add_executable(${PROJECT_NAME} main.cpp triangle.cpp)

if (WIN32)
    target_link_directories(${PROJECT_NAME} PUBLIC "${VULKAN_SDK}/Lib/" "${GLFW_ROOT}/lib-vc2022")
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw3.lib vulkan-1.lib)
elseif (LINUX)
    target_link_directories(${PROJECT_NAME} PUBLIC "${VULKAN_SDK}/lib/")
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw vulkan VkLayer_khronos_validation)
endif ()