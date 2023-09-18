#!/bin/bash

mkdir -p ./build
cd ./build
cmake -DCMAKE_BUILD_TYPE=Debug -DVULKAN_SDK_ROOT=$VULKAN_SDK_ROOT ..
cmake --build .