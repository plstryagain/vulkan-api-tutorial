add_executable(${PROJECT_NAME} main.cpp triangle.cpp utils.cpp)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/shaders/compile_win.bat
)

if (WIN32)
    target_link_directories(${PROJECT_NAME} PUBLIC "$ENV{VULKAN_SDK}/Lib/" "$ENV{GLFW_ROOT}/lib-vc2022")
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw3.lib vulkan-1.lib)
elseif (LINUX)
    target_link_directories(${PROJECT_NAME} PUBLIC "$ENV{VULKAN_SDK}/lib/")
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw vulkan VkLayer_khronos_validation)
endif ()