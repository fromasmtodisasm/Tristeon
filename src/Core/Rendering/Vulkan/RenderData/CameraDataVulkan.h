#pragma once
#include "Core/Rendering/RenderManager.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				class CameraDataVulkan : public CameraData
				{
				public:
					void onPreRender() override;
					void onPostRender() override;

					struct OnscreenData
					{
					} onscreen;

					struct OffscreenData
					{
						vk::CommandBuffer commandBuffer;
						vk::Semaphore semaphore;
					} offscreen;
				private:
				};
			}
		}
	}
}
