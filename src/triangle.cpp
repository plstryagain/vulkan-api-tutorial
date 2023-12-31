#include "triangle.h"
#include "utils.h"
#include "vulkan/vulkan_core.h"


#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <stdint.h>
#include <string>
#include <vector>
#include <thread>
#include <set>
#include <limits>
#include <algorithm>
#include <array>

inline static const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

inline static const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    inline static constexpr bool enable_validation_layers = false;
#else
    inline static constexpr bool enable_validation_layers = true;
#endif

TriangleApplication::~TriangleApplication()
{
    vkDestroySemaphore(device_, semaphore_image_available_, nullptr);
    vkDestroySemaphore(device_, semaphore_render_finished_, nullptr);
    vkDestroyFence(device_, fence_in_flight_, nullptr);
    vkDestroyCommandPool(device_, command_pool_, nullptr);
    for (auto framebuffer: swap_chain_framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    vkDestroyRenderPass(device_, render_pass_, nullptr);
    for (auto image_view: swap_chain_image_views_) {
        vkDestroyImageView(device_, image_view, nullptr);
    }
    vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
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
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
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
    app_info.apiVersion = VK_API_VERSION_1_3;

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
    std::cout << "Instance created" << std::endl;
}

void TriangleApplication::mainLoop() 
{
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        drawFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

void TriangleApplication::drawFrame()
{
    VkResult res =  vkWaitForFences(device_, 1, &fence_in_flight_, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to wait for fences, error: " + std::to_string(res));
    }
    vkResetFences(device_, 1, &fence_in_flight_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to reset fences, error: " + std::to_string(res));
    }

    uint32_t image_index = 0;
    vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, semaphore_image_available_, VK_NULL_HANDLE, &image_index);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to acquire next image, error: " + std::to_string(res));
    }
    res = vkResetCommandBuffer(command_buffer_, 0);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to reset command buffer, error: " + std::to_string(res));
    }
    recordCommandBuffer(command_buffer_, image_index);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {
        semaphore_image_available_
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    VkSemaphore signal_semaphores[] = {
        semaphore_render_finished_
    };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    res = vkQueueSubmit(graphics_queue_, 1, &submit_info, fence_in_flight_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer, error: " + std::to_string(res));
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = {
        swap_chain_
    };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    res = vkQueuePresentKHR(present_queue_, &present_info);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to queue present, error: " + std::to_string(res));
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
    std::cout << "Picked physical device: " << getPhysicalDeviceName(physical_device_) << std::endl;
}

std::string TriangleApplication::getPhysicalDeviceName(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(device, &device_props);
    return std::string{ device_props.deviceName };
}

bool TriangleApplication::isSuitableDevice(VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(physical_device, &device_props);

    VkPhysicalDeviceFeatures device_feats;
    vkGetPhysicalDeviceFeatures(physical_device, &device_feats);
    auto indices = findQueueFamilies(physical_device);
    bool is_dev_ext_support = isDeviceExtensionSupport(physical_device);
    bool swap_chain_adequate = false;
    if (is_dev_ext_support) {
        SwapChainSupportDetails details = querySwapChainSupport(physical_device);
        swap_chain_adequate = !details.formats.empty() && !details.present_modes.empty();
    }
    return indices.isComplete() && is_dev_ext_support && swap_chain_adequate;
}

bool TriangleApplication::isDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> ext_props{count};
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, ext_props.data());
    std::set<std::string_view> required_exts{device_extensions.begin(), device_extensions.end()};
    for (const auto& prop: ext_props) {
        required_exts.erase(prop.extensionName);
    }
    return required_exts.empty();
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
        VkBool32 present_support{false};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
        if (present_support) {
            indices.present_family = i;
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
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
    std::set<uint32_t> unique_queue_families = {
        indices.graphics_family.value(), indices.present_family.value()
    };
    for (const auto& queue_family: unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueCount = 1;
        queue_create_info.queueFamilyIndex = queue_family;
        float queue_priority = 1.0f;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.emplace_back(std::move(queue_create_info));
    }

    VkPhysicalDeviceFeatures feats{};

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pEnabledFeatures = &feats;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    VkResult res = vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create device, error: " + std::to_string(res));
    }
    vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);
    std::cout << "Logical device created" << std::endl;
}

void TriangleApplication::createSwapChain()
{
    SwapChainSupportDetails details = querySwapChainSupport(physical_device_);
    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(details.present_modes);
    VkExtent2D extent = chooseSwapExtent(details.capabilities);
    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        image_count = details.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physical_device_);
    uint32_t queue_family_indices[] = {
            indices.graphics_family.value(), indices.present_family.value()
    };
    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    VkResult res = vkCreateSwapchainKHR(device_, &create_info, nullptr, &swap_chain_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain, error: " + std::to_string(res));
    }
    vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr);
    swap_chain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, swap_chain_images_.data());
    swap_chain_image_format_ = surface_format.format;
    swap_chain_extent_ = extent;
    std::cout << "Swap chain created" << std::endl;
}

void TriangleApplication::createSurface()
{
    VkResult res = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface, error: " + std::to_string(res));
    }
    std::cout << "Surface created" << std::endl;
}

void TriangleApplication::createImageViews()
{
    swap_chain_image_views_.reserve(swap_chain_images_.size());
    for (size_t i = 0; i < swap_chain_images_.size(); ++i) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images_[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain_image_format_;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        VkImageView view = VK_NULL_HANDLE;
        VkResult res = vkCreateImageView(device_, &create_info, nullptr, &view);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views, error: " + std::to_string(res));
        }
        swap_chain_image_views_.push_back(std::move(view));
        std::cout << "ImageView created" << std::endl;
    }
}

void TriangleApplication::createRenderPass() 
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_image_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;



    VkResult res = vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass, error: " + std::to_string(res));
    }
    std::cout << "RenderPass created" << std::endl;
}

void TriangleApplication::createGraphicsPipeline()
{
#if defined(_WIN32)
    auto vert = utils::readFile("shaders\\vert.spv");
    std::cout << "vert size: " << vert.size() << std::endl;
    auto frag = utils::readFile("shaders\\frag.spv");
    std::cout << "vert size: " << frag.size() << std::endl;
#elif defined(__linux__)
    auto vert = utils::readFile("shaders/vert.spv");
    std::cout << "vert size: " << vert.size() << std::endl;
    auto frag = utils::readFile("shaders/frag.spv");
    std::cout << "vert size: " << frag.size() << std::endl;
#endif
    VkShaderModule vert_shader_module = createShaderModule(vert);
    VkShaderModule frag_shader_module = createShaderModule(frag);

    VkPipelineShaderStageCreateInfo vert_stage_info{};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_shader_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info{};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_shader_module;
    frag_stage_info.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
        vert_stage_info, 
        frag_stage_info
    };

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();
    
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info{};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.vertexBindingDescriptionCount = 0;
    vertex_input_state_info.pVertexBindingDescriptions = nullptr;
    vertex_input_state_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_info.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info{};
    input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

    VkViewport view_port{};
    view_port.x = 0.0f;
    view_port.y = 0.0f;
    view_port.height = static_cast<float>(swap_chain_extent_.height);
    view_port.width = static_cast<float>(swap_chain_extent_.width);
    view_port.minDepth = 0.0f;
    view_port.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = swap_chain_extent_;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &view_port;

    VkPipelineRasterizationStateCreateInfo rasterizer_info{};
    rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.0f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;
    rasterizer_info.depthBiasClamp = 0.0f;
    rasterizer_info.depthBiasConstantFactor = 0.0f;
    rasterizer_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_info{};
    multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.sampleShadingEnable = VK_FALSE;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_info.minSampleShading = 1.0f;
    multisample_state_info.pSampleMask = nullptr;
    multisample_state_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.blendConstants[0] = 0.0f;
    color_blend_info.blendConstants[1] = 0.0f;
    color_blend_info.blendConstants[2] = 0.0f;
    color_blend_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    VkResult res = vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout, error: " + std::to_string(res));
    }

    std::cout << "Pipeline layout created" << std::endl;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages.data();

    pipeline_info.pVertexInputState = &vertex_input_state_info;
    pipeline_info.pInputAssemblyState = &input_assembly_state_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisample_state_info;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = &dynamic_state_info;

    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    res = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline, error: " + std::to_string(res));
    }

    std::cout << "Pipeline created" << std::endl;

    vkDestroyShaderModule(device_, vert_shader_module, nullptr);
    vkDestroyShaderModule(device_, frag_shader_module, nullptr);
}

void TriangleApplication::createFramebuffers()
{
    swap_chain_framebuffers_.resize(swap_chain_image_views_.size());

    for (size_t i = 0; i < swap_chain_image_views_.size(); i++) {
        VkImageView attachments[] = {
            swap_chain_image_views_[i]
        };

        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass_;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = swap_chain_extent_.width;
        create_info.height = swap_chain_extent_.height;
        create_info.layers = 1;

        VkResult res = vkCreateFramebuffer(device_, &create_info, nullptr, &swap_chain_framebuffers_[i]);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer, error: " + std::to_string(res));
        }
    }
    std::cout << "Framebuffers created" << std::endl;
}

void TriangleApplication::createCommandPool()
{
    QueueFamilyIndices queue_family_indices = findQueueFamilies(physical_device_);
    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

    VkResult res = vkCreateCommandPool(device_, &create_info, nullptr, &command_pool_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool, error: " + std::to_string(res));
    } 
    std::cout << "Command pool created" << std::endl;
}

void TriangleApplication::createCommandBuffer()
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.commandBufferCount = 1;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkResult res = vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers, error: " + std::to_string(res));
    }
    std::cout << "Command buffer created" << std::endl;
}

void TriangleApplication::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult res = vkCreateSemaphore(device_, &semaphore_info, nullptr, &semaphore_image_available_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create image_available semaphore, error: " + std::to_string(res));
    }
    res = vkCreateSemaphore(device_, &semaphore_info, nullptr, &semaphore_render_finished_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create render_finished semaphore, error: " + std::to_string(res));
    }
    res = vkCreateFence(device_, &fence_info, nullptr, &fence_in_flight_);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create in_flight fence, error: " + std::to_string(res));
    }
}

void TriangleApplication::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    VkResult res = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer, error: " + std::to_string(res));
    }
    std::cout << "Command buffer record begin" << std::endl;

    VkRenderPassBeginInfo rp_begin_info{};
    rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin_info.renderPass = render_pass_;
    rp_begin_info.framebuffer = swap_chain_framebuffers_[image_index];
    rp_begin_info.renderArea.offset = {0, 0};
    rp_begin_info.renderArea.extent = swap_chain_extent_;

    VkClearValue color_value = {
        {
            { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };
    rp_begin_info.clearValueCount = 1;
    rp_begin_info.pClearValues = &color_value;

    vkCmdBeginRenderPass(command_buffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

    VkViewport view_port{};
    view_port.x = 0.0f;
    view_port.y = 0.0f;
    view_port.width = static_cast<float>(swap_chain_extent_.width);
    view_port.height = static_cast<float>(swap_chain_extent_.height);
    view_port.minDepth = 0.0f;
    view_port.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &view_port);

    VkRect2D scissor{};
    scissor.extent = swap_chain_extent_;
    scissor.offset = {0, 0};
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    res = vkEndCommandBuffer(command_buffer);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer, error: " + std::to_string(res));
    }
    std::cout << "Command buffer recorded" << std::endl;
}

SwapChainSupportDetails TriangleApplication::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }
    return details;
}

VkSurfaceFormatKHR TriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    for (const auto& format: available_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return available_formats[0];
}

VkPresentModeKHR TriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    for (const auto& present_mode: available_present_modes) {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actual_extent;
    }
}

VkShaderModule TriangleApplication::createShaderModule(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shader_module;
    VkResult res = vkCreateShaderModule(device_, &create_info, nullptr, &shader_module);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module, error: " + std::to_string(res));
    }
    return shader_module;
}
