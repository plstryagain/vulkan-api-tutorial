#include "triangle.h"



#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

inline static const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    inline static constexpr bool enable_validation_layers = false;
#else
    inline static constexpr bool enable_validation_layers = true;
#endif

TriangleApplication::~TriangleApplication()
{
    vkDestroyInstance(instance_, nullptr);
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void TriangleApplication::run() 
{
    initWindow();
    initVulkan();
    mainLoop();
}


void TriangleApplication::initWindow() 
{
    int rv = glfwInit();
    if (rv != GLFW_TRUE) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create window!" << std::endl;
    }
}

void TriangleApplication::initVulkan() 
{
    enumExtensions();
    createInstance();
    pickPhysicalDevice();
    createLogicalDevice();
}

void TriangleApplication::enumExtensions() 
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions{extension_count};
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    std::cout << "Available extensions: " << std::endl;
    for (const auto& extension : extensions) {
        std::cout << "\t" << extension.extensionName << std::endl;
    }
}

void TriangleApplication::createInstance() 
{
    if (enable_validation_layers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledExtensionCount = glfw_extensions_count;
    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    } 

    VkResult res = vkCreateInstance(&create_info, nullptr, &instance_);
    if (res != VK_SUCCESS) {
        std::cerr << "error: " << res << std::endl;
        throw std::runtime_error("Failed to create VK instance!");
    }
}

void TriangleApplication::mainLoop() 
{
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}

bool TriangleApplication::checkValidationLayerSupport()
{
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers{layer_count};
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    for (const auto& layer_name : validation_layers) {
        bool layer_found = false;
        for (const auto& layer_properties : available_layers) {
            if (std::string_view{layer_properties.layerName} == std::string_view{layer_name}) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            return false;
        }
    }

    return true;
}

void TriangleApplication::pickPhysicalDevice()
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices{count};
    vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    for (const auto& device: devices) {
        if (isSuitableDevice(device)) {
            physical_device_ = device;
            break;
        }
    }
    if (physical_device_ == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool TriangleApplication::isSuitableDevice(VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(physical_device, &device_props);

    VkPhysicalDeviceFeatures device_feats;
    vkGetPhysicalDeviceFeatures(physical_device, &device_feats);
    auto indices = findQueueFamilies(physical_device);
    return indices.isComplete();
}

QueueFamilyIndices TriangleApplication::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices{};
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> family_props{count};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, family_props.data());
    int i = 0;
    for (const auto& prop: family_props) {
        if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

void TriangleApplication::createLogicalDevice() 
{
    auto indices = findQueueFamilies(physical_device_);
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueCount = 1;
    queue_create_info.queueFamilyIndex = indices.graphics_family.value();
    float queue_prioriy = 1.0f;
    queue_create_info.pQueuePriorities = &queue_prioriy;

    VkPhysicalDeviceFeatures feats{};

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pEnabledFeatures = &feats;
    device_create_info.enabledExtensionCount = 0;

    VkResult res = vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create device, error: " + std::to_string(res));
    }
}
