#include "triangle.h"

int main()
{
    std::cout << "VULKAN_SDK: " << std::getenv("VULKAN_SDK") << std::endl;
    TriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}