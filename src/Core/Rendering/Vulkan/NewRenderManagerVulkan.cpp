﻿#include "NewRenderManagerVulkan.h"
#include "Core/BindingData.h"

#include <Core/Rendering/Vulkan/MaterialVulkan.h>

#include <Core/Rendering/Vulkan/RenderData/CameraDataVulkan.h>
#include <Core/Rendering/Vulkan/RenderData/MaterialDataVulkan.h>

#include "HelperClasses/QueueFamilyIndices.h"
#include "HelperClasses/VulkanFormat.h"
#include "HelperClasses/Pipeline.h"
#include "Editor/JsonSerializer.h"
#include "DebugDrawManagerVulkan.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				NewRenderManager::NewRenderManager()
				{
					initVulkan();

					graphicsState = IDLING;
				}

				void NewRenderManager::reflectShader(ShaderFile& file)
				{
				}

				void NewRenderManager::recompileShader(ShaderFile shader)
				{
					Pipeline* pipeline = *find_if(pipelines.begin(), pipelines.end(), [&](Pipeline* item) { return item->getShaderFile() == shader; });
					pipeline->recompile(shader);
				}

				std::unique_ptr<CameraData> NewRenderManager::createCameraData(Components::Camera* camera)
				{
					return std::make_unique<CameraDataVulkan>();
				}

				std::unique_ptr<MaterialData> NewRenderManager::createMaterialData(std::unique_ptr<Rendering::Material> material)
				{
					return std::make_unique<MaterialDataVulkan>();
				}

				std::unique_ptr<Rendering::Material> NewRenderManager::createMaterial(std::string filePath)
				{
					std::unique_ptr<Material> mat = std::unique_ptr<Material>(JsonSerializer::deserialize<Vulkan::Material>(filePath));
					return move(mat);
				}

				void NewRenderManager::resizeWindow(int w, int h)
				{
					if (w <= 0 || h <= 0)
						return;

					windowContext->resize(w, h);

					vk::Device device = context->getDevice();

					device.destroyRenderPass(offscreenPass);
					createOffscreenPass();

					//Resize materials
					for (size_t i = 0; i < pipelines.size(); i++)
						pipelines[i]->onResize(context->getExtent(), offscreenPass);

					//Resize renderers
					for(size_t i = 0; i < materials.size(); i++)
						for (size_t j = 0; j < materials[i]->renderers.size(); j++)
							materials[i]->renderers[j]->onResize();

					for (size_t i = 0; i < looseRenderers.size(); i++)
						looseRenderers[i]->onResize();

					//Resize cameras
					for (size_t i = 0; i < cameras.size(); i++)
						cameras[i]->onResize();


					DebugDrawManager::onResize();
					
					context->getSwapchain()->createFramebuffers();
				}

				void NewRenderManager::onPostRender()
				{
					submitCameras();
				}

				void NewRenderManager::initVulkan()
				{
					context = new WindowContextVulkan(reinterpret_cast<Window*>(BindingData::getInstance()->tristeonWindow));
					windowContext = std::unique_ptr<WindowContextVulkan>(context);

					VulkanBindingData* bindingData = VulkanBindingData::getInstance();
					bindingData->readFromWindowContext(context);

					createDescriptorPool();
					createCommandPool();

					createOffscreenPass();
					createOnscreenPipeline();

					context->getSwapchain()->createFramebuffers();
					createPrimaryCommandBuffer();

					//Initialize textures and descriptorsets
					for (const auto& m : materials)
						m->material->resetProperties();
				}

				void NewRenderManager::createDescriptorPool()
				{
					vk::DescriptorPoolSize const buffers = vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000);
					vk::DescriptorPoolSize const samplers = vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000);

					std::array<vk::DescriptorPoolSize, 2> sizes = { buffers, samplers };

					vk::DescriptorPoolCreateInfo const ci = vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, sizes.size(), sizes.data());

					descriptorPool = context->getDevice().createDescriptorPool(ci);

					VulkanBindingData::getInstance()->descriptorPool = descriptorPool;
				}

				void NewRenderManager::createCommandPool()
				{
					const QueueFamilyIndices indices = QueueFamilyIndices::get(context->getGPU(), context->getSurfaceKHR());

					vk::CommandPoolCreateInfo const ci = vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, indices.graphicsFamily);
					commandPool = context->getDevice().createCommandPool(ci);

					VulkanBindingData::getInstance()->commandPool = commandPool;
				}

				void NewRenderManager::createOffscreenPass()
				{
					//Color attachment 
					vk::AttachmentDescription const color = vk::AttachmentDescription({},
						vk::Format::eR8G8B8A8Unorm,
						vk::SampleCountFlagBits::e1,
						vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
						vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
						vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);

					//The offscreen pass renders 3D data and as such it uses a depth buffer
					vk::AttachmentDescription const depth = vk::AttachmentDescription({},
						VulkanFormat::findDepthFormat(context->getGPU()),
						vk::SampleCountFlagBits::e1,
						vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
						vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
						vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

					//Attachments
					std::array<vk::AttachmentDescription, 2> attachmentDescr = { color, depth };
					vk::AttachmentReference colorRef = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
					vk::AttachmentReference depthRef = vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

					//Subpass takes the references to our attachments
					vk::SubpassDescription subpass = vk::SubpassDescription({}, vk::PipelineBindPoint::eGraphics,
						0, nullptr,
						1, &colorRef,
						nullptr, &depthRef,
						0, nullptr);

					//Dependencies
					std::array<vk::SubpassDependency, 2> dependencies = {
						vk::SubpassDependency(
							VK_SUBPASS_EXTERNAL, 0,
							vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
							vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
							vk::DependencyFlagBits::eByRegion),

						vk::SubpassDependency(
							0, VK_SUBPASS_EXTERNAL,
							vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
							vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
							vk::DependencyFlagBits::eByRegion)
					};

					//Create the renderpass
					vk::RenderPassCreateInfo const rp = vk::RenderPassCreateInfo({},
						attachmentDescr.size(), attachmentDescr.data(),
						1, &subpass,
						dependencies.size(), dependencies.data()
					);
					offscreenPass = context->getDevice().createRenderPass(rp);
				}

				void NewRenderManager::createOnscreenPipeline()
				{
					ShaderFile const file = ShaderFile("Screen", "Files/Shaders/", "ScreenV", "ScreenF");
					onscreenPipeline = new Pipeline(file, context->getExtent(), context->getRenderpass(), false, vk::PrimitiveTopology::eTriangleList, false, vk::CullModeFlagBits::eFront);
				}

				void NewRenderManager::createPrimaryCommandBuffer()
				{
					vk::CommandBufferAllocateInfo alloc = vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
					const vk::Result r = context->getDevice().allocateCommandBuffers(&alloc, &primaryCmd);
					Misc::Console::t_assert(r == vk::Result::eSuccess, "Failed to allocate command buffers: " + to_string(r));
				}

				void NewRenderManager::submitCameras()
				{
					vk::Semaphore imageAvailable = context->getImageAvailable();

					//Wait till present queue is ready to receive more commands
					context->getPresentQueue().waitIdle();
					vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

					CameraDataVulkan* last = nullptr;
					//Submit cameras
					for (const std::unique_ptr<CameraData> &p : cameras)
					{
						if (!inPlayMode && !p->camera->runInEditorMode)
							continue;

						CameraDataVulkan* c = dynamic_cast<CameraDataVulkan*>(p.get());
						vk::Semaphore wait = last == nullptr ? imageAvailable : last->offscreen.semaphore;
						vk::SubmitInfo s = vk::SubmitInfo(1, &wait, waitStages, 1, &c->offscreen.commandBuffer, 1, &c->offscreen.semaphore);
						context->getGraphicsQueue().submit(1, &s, nullptr);
						last = c;
					}

					//Submit onscreen
					vk::Semaphore renderFinished = context->getRenderFinished();
					vk::SubmitInfo s2 = vk::SubmitInfo(
						1, last == nullptr ? &imageAvailable : &last->offscreen.semaphore,
						waitStages,
						1, &primaryCmd,
						1, &renderFinished);
					context->getGraphicsQueue().submit(1, &s2, nullptr);
				}
			}
		}
	}
}
