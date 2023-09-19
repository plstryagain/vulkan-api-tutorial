add_executable(${PROJECT_NAME} main.cpp triangle.cpp)

if (WIN32)
    target_link_directories(${PROJECT_NAME} PUBLIC "${VULKAN_SDK_ROOT}/Lib/" "${GLFW_ROOT}/lib-vc2022")
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw3.lib vulkan-1.lib)
elseif (LINUX)
    target_link_directories(${PROJECT_NAME} PUBLIC "${VULKAN_SDK_ROOT}/x86_64/lib/")
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw vulkan)
endif ()