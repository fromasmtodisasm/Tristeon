#pragma once
#include <Core/Rendering/Internal/InternalRenderer.h>

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				class InternalSkyboxRenderer : public InternalRenderer
				{
				public:
					explicit InternalSkyboxRenderer(Renderer* pOwner);
					void render() override;
				};
			}
		}
	}
}