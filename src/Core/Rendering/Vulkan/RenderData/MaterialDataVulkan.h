#pragma once
#include "CameraDataVulkan.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				class MaterialDataVulkan : public MaterialData
				{
				public:
					void bind() override;
					void unBind() override;
				};
			}
		}
	}
}