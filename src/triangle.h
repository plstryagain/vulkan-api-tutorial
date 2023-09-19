#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>


class TriangleApplication {
public:
    TriangleApplication() = default;
    ~TriangleApplication();
public:
    void run();


private:
    void initWindow();
    void initVulkan();
    void enumExtensions();
    void createInstance();
    void mainLoop();
    bool checkValidationLayerSupport();

private:
    GLFWwindow* window_ = nullptr;
    VkInstance instance_;
};