#include "Graphics.h"
#include "RenderManager.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			RenderManager* Graphics::renderer = nullptr;

			Material* Graphics::getMaterial(std::string filePath)
			{
				if (!renderer)
					throw std::exception("The renderer is null upon calling getMaterial()!");

				return renderer->getMaterial(filePath);
			}

			void Graphics::reflectShader(ShaderFile& file)
			{
				if (!renderer)
					throw std::exception("The renderer is null upon calling Graphics::reflectShader()!");

				return renderer->reflectShader(file);
			}

			void Graphics::setMaterialsDirty()
			{
				renderer->setMaterialsDirty();
			}
		}
	}
}