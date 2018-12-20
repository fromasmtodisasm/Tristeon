#pragma once
#include <string>

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			class ShaderFile;
			class Material;
			class RenderManager;

			/**
			 * Static API for interacting with the rendering/graphics backend of Tristeon.
			 * Should never be used from a static constructor.
			 */
			class Graphics
			{
				friend RenderManager;
			public:
				/**
				 * Gets the material that's at the given filepath.
				 * Returns a cached value unless if the material hadn't been loaded in before.
				 * \exception exception When the renderer is null. Only possible if a Graphics function is called from a static constructor.
				 * \return The material object if found
				 * \return nullptr if the material wasn't found
				 */
				static Material* getMaterial(std::string filePath);

				/**
				 * Reflects the shader and populates ShaderFile::properties of the given file.
				 * ShaderFile::properties will be empty if the compilation or reflection failed.
				 * \exception exception When the renderer is null. Only possible if a Graphics function is called from a static constructor.
				 */
				static void reflectShader(ShaderFile& file);

				static void setMaterialsDirty();
			private:
				static RenderManager* renderer;
			};
		}
	}
}