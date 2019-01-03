#pragma once
#include <string>

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			class Renderer;
			class ShaderFile;
			class Material;
			class RenderManager;

			enum GraphicsState
			{
				INITIALIZING,
				IDLING,
				RENDERING,
				TERMINATING
			};

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
				 * \return nullptr If the material wasn't found
				 */
				static Material* getMaterial(std::string filePath);

				/**
				 * Reflects the shader and populates ShaderFile::properties of the given file.
				 * ShaderFile::properties will be empty if the compilation or reflection failed.
				 * \exception exception When the renderer is null. Only possible if a Graphics function is called from a static constructor.
				 */
				static void reflectShader(ShaderFile& file);

				/**
				 * Sets the materialsDirty flag. Which forces materials to be re-sorted based on their renderqueue.
				 * \exception exception When the renderer is null. Only possible if a Graphics function is called from a static constructor.
				 */
				static void setMaterialsDirty();

				/**
				 * Re-adds the given renderer to the correct material queue.
				 * \exception invalid_argument The passed renderer is null
				 * \exception runtime_error Couldn't find material data for oldMat
				 * \exception runtime_error Couldn't find material data for newMat
				 * \exception exception When the renderer is null. Only possible if a Graphics function is called from a static constructor.
				 */
				static void materialChanged(Renderer* renderer, Material* oldMat, Material* newMat);

				static GraphicsState getGraphicsState();
			private:
				static RenderManager* renderer;
			};
		}
	}
}