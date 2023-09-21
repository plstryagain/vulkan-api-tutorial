#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <optional>
#include <string>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool isComplete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

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
    void pickPhysicalDevice();
    bool isSuitableDevice(VkPhysicalDevice physical_device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createLogicalDevice();
    void createSurface();
    std::string TriangleApplication::getPhysicalDeviceName(VkPhysicalDevice device);

private:
    GLFWwindow* window_ = nullptr;
    VkInstance instance_;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
};