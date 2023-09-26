$Env:VULKAN_SDK_ROOT
mkdir -Force ./build
cd ./build
# cmake -DCMAKE_BUILD_TYPE=Debug -DVULKAN_SDK="$Env:VULKAN_SDK" -DGLM_ROOT="$Env:GLM_ROOT" -DGLFW_ROOT="$Env:GLFW_ROOT" -G "Visual Studio 17 2022" ..
cmake -DCMAKE_BUILD_TYPE=Debug -DVULKAN_SDK="$Env:VULKAN_SDK" -DGLM_ROOT="$Env:GLM_ROOT" -DGLFW_ROOT="$Env:GLFW_ROOT" -DCMAKE_MAKE_PROGRAM="C:\\Program Files\\Ninja\\ninja.exe" -G "Ninja" ..
cmake --build .