#include <Core/BindingData.h>

namespace Tristeon
{
	namespace Core
	{
		//Definition is required for our static instance to work
		std::unique_ptr<BindingData> BindingData::instance;

		void VulkanBindingData::readFromWindowContext(Rendering::Vulkan::WindowContextVulkan* context)
		{
			physicalDevice = context->getGPU();
			device = context->getDevice();
			graphicsQueue = context->getGraphicsQueue();
			presentQueue = context->getPresentQueue();

			swapchain = context->getSwapchain();
			renderPass = context->getSwapchain()->renderpass;
		}
	}
}