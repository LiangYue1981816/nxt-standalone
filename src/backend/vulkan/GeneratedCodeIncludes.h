// Copyright 2017 The NXT Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "backend/vulkan/BindGroupLayoutVk.h"
#include "backend/vulkan/BindGroupVk.h"
#include "backend/vulkan/BlendStateVk.h"
#include "backend/vulkan/BufferVk.h"
#include "backend/vulkan/CommandBufferVk.h"
#include "backend/vulkan/ComputePipelineVk.h"
#include "backend/vulkan/DepthStencilStateVk.h"
#include "backend/vulkan/InputStateVk.h"
#include "backend/vulkan/PipelineLayoutVk.h"
#include "backend/vulkan/RenderPassDescriptorVk.h"
#include "backend/vulkan/RenderPipelineVk.h"
#include "backend/vulkan/SamplerVk.h"
#include "backend/vulkan/ShaderModuleVk.h"
#include "backend/vulkan/SwapChainVk.h"
#include "backend/vulkan/TextureVk.h"
#include "backend/vulkan/VulkanBackend.h"
