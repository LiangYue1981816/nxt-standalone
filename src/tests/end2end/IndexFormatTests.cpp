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

#include "tests/NXTTest.h"

#include "common/Assert.h"
#include "utils/NXTHelpers.h"

constexpr uint32_t kRTSize = 400;

class IndexFormatTest : public NXTTest {
    protected:
        void SetUp() override {
            NXTTest::SetUp();

            renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);
        }

        utils::BasicRenderPass renderPass;

        nxt::RenderPipeline MakeTestPipeline(nxt::IndexFormat format) {
            nxt::InputState inputState = device.CreateInputStateBuilder()
                .SetInput(0, 4 * sizeof(float), nxt::InputStepMode::Vertex)
                .SetAttribute(0, 0, nxt::VertexFormat::FloatR32G32B32A32, 0)
                .GetResult();

            nxt::ShaderModule vsModule = utils::CreateShaderModule(device, nxt::ShaderStage::Vertex, R"(
                #version 450
                layout(location = 0) in vec4 pos;
                void main() {
                    gl_Position = pos;
                })"
            );

            nxt::ShaderModule fsModule = utils::CreateShaderModule(device, nxt::ShaderStage::Fragment, R"(
                #version 450
                layout(location = 0) out vec4 fragColor;
                void main() {
                    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
                })"
            );

            return device.CreateRenderPipelineBuilder()
                .SetColorAttachmentFormat(0, renderPass.colorFormat)
                .SetPrimitiveTopology(nxt::PrimitiveTopology::TriangleStrip)
                .SetStage(nxt::ShaderStage::Vertex, vsModule, "main")
                .SetStage(nxt::ShaderStage::Fragment, fsModule, "main")
                .SetIndexFormat(format)
                .SetInputState(inputState)
                .GetResult();
        }
};

// Test that the Uint32 index format is correctly interpreted
TEST_P(IndexFormatTest, Uint32) {
    nxt::RenderPipeline pipeline = MakeTestPipeline(nxt::IndexFormat::Uint32);

    nxt::Buffer vertexBuffer = utils::CreateFrozenBufferFromData<float>(device, nxt::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f, // Note Vertices[0] = Vertices[1]
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    // If this is interpreted as Uint16, then it would be 0, 1, 0, ... and would draw nothing.
    nxt::Buffer indexBuffer = utils::CreateFrozenBufferFromData<uint32_t>(device, nxt::BufferUsageBit::Index, {
        1, 2, 3
    });

    uint32_t zeroOffset = 0;
    nxt::CommandBuffer commands = device.CreateCommandBufferBuilder()
        .BeginRenderPass(renderPass.renderPassInfo)
            .SetRenderPipeline(pipeline)
            .SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset)
            .SetIndexBuffer(indexBuffer, 0)
            .DrawElements(3, 1, 0, 0)
        .EndRenderPass()
        .GetResult();

    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

// Test that the Uint16 index format is correctly interpreted
TEST_P(IndexFormatTest, Uint16) {
    nxt::RenderPipeline pipeline = MakeTestPipeline(nxt::IndexFormat::Uint16);

    nxt::Buffer vertexBuffer = utils::CreateFrozenBufferFromData<float>(device, nxt::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    // If this is interpreted as uint32, it will have index 1 and 2 be both 0 and render nothing
    nxt::Buffer indexBuffer = utils::CreateFrozenBufferFromData<uint16_t>(device, nxt::BufferUsageBit::Index, {
        1, 2, 0, 0, 0, 0
    });

    uint32_t zeroOffset = 0;
    nxt::CommandBuffer commands = device.CreateCommandBufferBuilder()
        .BeginRenderPass(renderPass.renderPassInfo)
            .SetRenderPipeline(pipeline)
            .SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset)
            .SetIndexBuffer(indexBuffer, 0)
            .DrawElements(3, 1, 0, 0)
        .EndRenderPass()
        .GetResult();

    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

// Test for primitive restart use vertices like in the drawing and draw the following
// indices: 0 1 2 PRIM_RESTART 3 4 2. Then A and B should be written but not C.
//      |--------------|
//      |      0       |
//      |      |\      |
//      |      |B \    |
//      |      2---1   |
//      |     /| C     |
//      |   / A|       |
//      |  4---3       |
//      |--------------|

// Test use of primitive restart with an Uint32 index format
TEST_P(IndexFormatTest, Uint32PrimitiveRestart) {
    nxt::RenderPipeline pipeline = MakeTestPipeline(nxt::IndexFormat::Uint32);

    nxt::Buffer vertexBuffer = utils::CreateFrozenBufferFromData<float>(device, nxt::BufferUsageBit::Vertex, {
         0.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    });
    nxt::Buffer indexBuffer = utils::CreateFrozenBufferFromData<uint32_t>(device, nxt::BufferUsageBit::Index, {
        0, 1, 2, 0xFFFFFFFFu, 3, 4, 2,
    });

    uint32_t zeroOffset = 0;
    nxt::CommandBuffer commands = device.CreateCommandBufferBuilder()
        .BeginRenderPass(renderPass.renderPassInfo)
            .SetRenderPipeline(pipeline)
            .SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset)
            .SetIndexBuffer(indexBuffer, 0)
            .DrawElements(7, 1, 0, 0)
        .EndRenderPass()
        .GetResult();

    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 190, 190);  // A
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 210, 210);  // B
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), renderPass.color, 210, 190);      // C
}

// Test use of primitive restart with an Uint16 index format
TEST_P(IndexFormatTest, Uint16PrimitiveRestart) {
    nxt::RenderPipeline pipeline = MakeTestPipeline(nxt::IndexFormat::Uint16);

    nxt::Buffer vertexBuffer = utils::CreateFrozenBufferFromData<float>(device, nxt::BufferUsageBit::Vertex, {
         0.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  0.0f, 0.0f, 1.0f,
         0.0f,  0.0f, 0.0f, 1.0f,
         0.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    });
    nxt::Buffer indexBuffer = utils::CreateFrozenBufferFromData<uint16_t>(device, nxt::BufferUsageBit::Index, {
        0, 1, 2, 0xFFFFu, 3, 4, 2,
    });

    uint32_t zeroOffset = 0;
    nxt::CommandBuffer commands = device.CreateCommandBufferBuilder()
        .BeginRenderPass(renderPass.renderPassInfo)
            .SetRenderPipeline(pipeline)
            .SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset)
            .SetIndexBuffer(indexBuffer, 0)
            .DrawElements(7, 1, 0, 0)
        .EndRenderPass()
        .GetResult();

    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 190, 190);  // A
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 210, 210);  // B
    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 0, 0, 0), renderPass.color, 210, 190);      // C
}

// Test that the index format used is the format of the last set pipeline. This is to
// prevent a case in D3D12 where the index format would be captured from the last
// pipeline on SetIndexBuffer.
TEST_P(IndexFormatTest, ChangePipelineAfterSetIndexBuffer) {
    if (IsD3D12() || IsVulkan()) {
        std::cout << "Test skipped on D3D12 and Vulkan" << std::endl;
        return;
    }

    nxt::RenderPipeline pipeline32 = MakeTestPipeline(nxt::IndexFormat::Uint32);
    nxt::RenderPipeline pipeline16 = MakeTestPipeline(nxt::IndexFormat::Uint16);

    nxt::Buffer vertexBuffer = utils::CreateFrozenBufferFromData<float>(device, nxt::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f, // Note Vertices[0] = Vertices[1]
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    // If this is interpreted as Uint16, then it would be 0, 1, 0, ... and would draw nothing.
    nxt::Buffer indexBuffer = utils::CreateFrozenBufferFromData<uint32_t>(device, nxt::BufferUsageBit::Index, {
        1, 2, 3
    });

    uint32_t zeroOffset = 0;
    nxt::CommandBuffer commands = device.CreateCommandBufferBuilder()
        .BeginRenderPass(renderPass.renderPassInfo)
            .SetRenderPipeline(pipeline16)
            .SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset)
            .SetIndexBuffer(indexBuffer, 0)
            .SetRenderPipeline(pipeline32)
            .DrawElements(3, 1, 0, 0)
        .EndRenderPass()
        .GetResult();

    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

// Test that setting the index buffer before the pipeline works, this is important
// for backends where the index format is passed inside the call to SetIndexBuffer
// because it needs to be done lazily (to query the format from the last pipeline).
// TODO(cwallez@chromium.org): This is currently disallowed by the validation but
// we want to support eventually.
TEST_P(IndexFormatTest, DISABLED_SetIndexBufferBeforeSetPipeline) {
    nxt::RenderPipeline pipeline = MakeTestPipeline(nxt::IndexFormat::Uint32);

    nxt::Buffer vertexBuffer = utils::CreateFrozenBufferFromData<float>(device, nxt::BufferUsageBit::Vertex, {
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    });
    nxt::Buffer indexBuffer = utils::CreateFrozenBufferFromData<uint32_t>(device, nxt::BufferUsageBit::Index, {
        0, 1, 2
    });

    uint32_t zeroOffset = 0;
    nxt::CommandBuffer commands = device.CreateCommandBufferBuilder()
        .BeginRenderPass(renderPass.renderPassInfo)
            .SetIndexBuffer(indexBuffer, 0)
            .SetRenderPipeline(pipeline)
            .SetVertexBuffers(0, 1, &vertexBuffer, &zeroOffset)
            .DrawElements(3, 1, 0, 0)
        .EndRenderPass()
        .GetResult();

    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(RGBA8(0, 255, 0, 255), renderPass.color, 100, 300);
}

NXT_INSTANTIATE_TEST(IndexFormatTest, D3D12Backend, MetalBackend, OpenGLBackend, VulkanBackend)
