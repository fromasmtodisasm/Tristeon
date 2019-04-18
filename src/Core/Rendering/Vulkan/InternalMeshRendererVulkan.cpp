#include "InternalMeshRendererVulkan.h"
#include "MaterialVulkan.h"

#include "Core/Transform.h"
#include "Core/BindingData.h"

#include "Misc/Console.h"
#include "Core/Rendering/Components/Renderer.h"
#include "Core/Rendering/Components/MeshRenderer.h"

#include "HelperClasses/Pipeline.h"
#include "Core/GameObject.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				InternalMeshRenderer::InternalMeshRenderer(MeshRenderer* renderer) : InternalRenderer(renderer), meshRenderer(renderer)
				{
					//We expect a meshrenderer
					meshRenderer = dynamic_cast<MeshRenderer*>(renderer);

					//Store rendering data
					createCommandBuffers();
					createUniformBuffer();
					createDescriptorSets();
					createVertexBuffer(meshRenderer->mesh.get());
					createIndexBuffer(meshRenderer->mesh.get());
				}

				InternalMeshRenderer::~InternalMeshRenderer()
				{
					//Cleanup
					VulkanBindingData* bindingData = VulkanBindingData::getInstance();
					bindingData->device.waitIdle();
					bindingData->device.freeDescriptorSets(bindingData->descriptorPool, 1, &set);
				}

				void InternalMeshRenderer::render()
				{
					//Check mesh
					if (meshRenderer->mesh.get().vertices.size() == 0 || meshRenderer->mesh.get().indices.size() == 0)
						return;

					//Check buffers
					if ((VkBuffer)vertexBuffer->getBuffer() == VK_NULL_HANDLE || (VkBuffer)indexBuffer->getBuffer() == VK_NULL_HANDLE)
					{
						Misc::Console::warning("Not rendering [" + meshRenderer->gameObject.get()->name + "] because either the vertex or index buffer hasn't been set up!");
						return;
					}

					//Check material
					Material* material = dynamic_cast<Material*>(meshRenderer->material.get());
					if (material == nullptr)
						return;

					//Check material descriptors
					if ((VkDescriptorSet)set == VK_NULL_HANDLE || (VkDescriptorSet)material->set == VK_NULL_HANDLE)
						return;

					//Bind material data
					material->bindInstanceMemory(uniformBuffer->getDeviceMemory());

					//Start secondary cmd buffer
					const vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &data->inheritance);
					vk::CommandBuffer secondary = cmd;

					secondary.begin(beginInfo);
					secondary.setViewport(0, 1, &data->viewport);
					secondary.setScissor(0, 1, &data->scissor);
					secondary.bindPipeline(vk::PipelineBindPoint::eGraphics, material->getPipeline()->getPipeline());

					//Descriptor sets
					std::vector<vk::DescriptorSet> sets = { set, material->getDescriptorSet() };
					if (data->skyboxSet && material->getPipeline()->getEnableLighting())
						sets.push_back(data->skyboxSet);

					secondary.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, material->getPipeline()->getPipelineLayout(), 0, sets.size(), sets.data(), 0, nullptr);

					//Bind buffers
					vk::Buffer vertexBuffers[] = { vertexBuffer->getBuffer() };
					vk::DeviceSize offsets[1] = { 0 };
					secondary.bindVertexBuffers(0, 1, vertexBuffers, offsets);
					secondary.bindIndexBuffer(indexBuffer->getBuffer(), 0, vk::IndexType::eUint16);

					//Draw and finish
					secondary.setLineWidth(2);
					secondary.drawIndexed(meshRenderer->mesh.get().indices.size(), 1, 0, 0, 0);
					secondary.end();

					data->lastUsedSecondaryBuffer = cmd;
				}

				void InternalMeshRenderer::onMeshChange(Data::SubMesh mesh)
				{
					createVertexBuffer(mesh);
					createIndexBuffer(mesh);
				}

				void InternalMeshRenderer::createCommandBuffers()
				{
					vk::CommandBufferAllocateInfo alloc = vk::CommandBufferAllocateInfo(VulkanBindingData::getInstance()->commandPool, vk::CommandBufferLevel::eSecondary, 1);
					vk::Result const r = VulkanBindingData::getInstance()->device.allocateCommandBuffers(&alloc, &cmd);
					Misc::Console::t_assert(r == vk::Result::eSuccess, "Failed to allocate command buffers: " + to_string(r));
				}

				void InternalMeshRenderer::createVertexBuffer(Data::SubMesh mesh)
				{
					vk::DeviceSize const size = sizeof(Data::Vertex) * mesh.vertices.size();
					if (size == 0)
						return;

					vertexBuffer = BufferVulkan::createOptimized(size, mesh.vertices.data(), vk::BufferUsageFlagBits::eVertexBuffer);
				}

				void InternalMeshRenderer::createIndexBuffer(Data::SubMesh mesh)
				{
					vk::DeviceSize const size = sizeof(uint16_t) * mesh.indices.size();
					if (size == 0)
						return;
					indexBuffer = BufferVulkan::createOptimized(size, mesh.indices.data(), vk::BufferUsageFlagBits::eIndexBuffer);
				}

				void InternalMeshRenderer::createUniformBuffer()
				{
					uniformBuffer = std::make_unique<BufferVulkan>(sizeof(UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, 
						vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
				}

				void InternalMeshRenderer::createDescriptorSets()
				{
					VulkanBindingData* bindingData = VulkanBindingData::getInstance();

					//Create a temporary layout describing the uniform buffer input
					vk::DescriptorSetLayout layout;
					vk::DescriptorSetLayoutBinding const ubo = vk::DescriptorSetLayoutBinding(
						0, vk::DescriptorType::eUniformBuffer,
						1, vk::ShaderStageFlagBits::eVertex,
						nullptr);
					vk::DescriptorSetLayoutCreateInfo ci = vk::DescriptorSetLayoutCreateInfo({}, 1, &ubo);
					bindingData->device.createDescriptorSetLayout(&ci, nullptr, &layout);

					//Allocate
					vk::DescriptorSetAllocateInfo alloc = vk::DescriptorSetAllocateInfo(bindingData->descriptorPool, 1, &layout);
					vk::Result const r = bindingData->device.allocateDescriptorSets(&alloc, &set);
					Misc::Console::t_assert(r == vk::Result::eSuccess, "Failed to allocate descriptor set!");

					//Write info
					vk::DescriptorBufferInfo buffer = vk::DescriptorBufferInfo(uniformBuffer->getBuffer(), 0, sizeof(UniformBufferObject));
					vk::WriteDescriptorSet const uboWrite = vk::WriteDescriptorSet(set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &buffer, nullptr);

					//Update descriptor with our new write info
					std::array<vk::WriteDescriptorSet, 1> write = { uboWrite };
					bindingData->device.updateDescriptorSets(write.size(), write.data(), 0, nullptr);

					bindingData->device.destroyDescriptorSetLayout(layout);
				}
			}
		}
	}
}
