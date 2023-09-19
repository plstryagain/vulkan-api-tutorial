$Env:VULKAN_SDK_ROOT
mkdir -Force ./build
cd ./build
cmake -DCMAKE_BUILD_TYPE=Debug -DVULKAN_SDK_ROOT="$Env:VULKAN_SDK_ROOT" -DGLM_ROOT="$Env:GLM_ROOT" -DGLFW_ROOT="$Env:GLFW_ROOT" -G "Visual Studio 17 2022" ..
cmake --build .