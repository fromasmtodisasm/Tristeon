#pragma once
#include "Core/Rendering/RenderManager.h"
#include "API/WindowContextVulkan.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				class Pipeline;

				class NewRenderManager : public Rendering::RenderManager
				{
				public:
					NewRenderManager();

					void reflectShader(ShaderFile& file) override;
				protected:
					void recompileShader(ShaderFile shader) override;
					std::unique_ptr<CameraData> createCameraData(Components::Camera* camera) override;
					std::unique_ptr<MaterialData> createMaterialData(std::unique_ptr<Rendering::Material> material) override;
					std::unique_ptr<Rendering::Material> createMaterial(std::string filePath) override;
					void resizeWindow(int w, int h) override;
					void onPreRender() override;
					void onPostRender() override;

				private:
					void initVulkan();
					void createDescriptorPool();
					void createCommandPool();
					void createOffscreenPass();
					void createOnscreenPipeline();
					void createPrimaryCommandBuffer();
					WindowContextVulkan* context;

					vk::DescriptorPool descriptorPool;
					vk::CommandPool commandPool;
					vk::RenderPass offscreenPass;
					Pipeline* onscreenPipeline;
					vk::CommandBuffer primaryCmd;

					vector<Pipeline*> pipelines;
				};
			}
		}
	}
}