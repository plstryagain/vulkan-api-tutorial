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
