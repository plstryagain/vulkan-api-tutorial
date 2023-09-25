#!/bin/bash

$VULKAN_SDK/bin/glslc ../../shaders/triangle_shader.vert -o ../../shaders/vert.spv
$VULKAN_SDK/bin/glslc ../../shaders/triangle_shader.frag -o ../../shaders/frag.spv