#pragma once
#include <Misc/vector.h>
#include <Core/Rendering/ShaderFile.h>
#include <Core/TObject.h>
#include <Core/Rendering/Graphics.h>
#include "RenderData/MaterialData.h"
#include "RenderData/CameraData.h"
#include "Interface/WindowContext.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Components { class Camera; }
		namespace Rendering
		{
			class Renderer; 
			class Material;

			class RenderManager
			{
				friend Graphics;
			public:
				/**
				 * Registers the rendermanager to the MessageBus callbacks
				 */
				RenderManager();
				virtual ~RenderManager();
				/**
				 * Gets the material that's at the given filepath. 
				 * Returns a cached value unless if the material hadn't been loaded in before.
				 * \return The material object if found
				 * \return nullptr if the material wasn't found
				 */
				Material* getMaterial(std::string filePath);

				/**
				 * Reflects the shader and populates ShaderFile::properties of the given file.
				 * ShaderFile::properties will be empty if the compilation or reflection failed.
				 */
				virtual void reflectShader(ShaderFile& file) = 0;

				/**
				 * Sets the materialsDirty flag. Which forces materials to be re-sorted based on their renderqueue.
				 */
				void setMaterialsDirty() { materialsDirty = true; }

				/**
				 * Re-adds the given renderer to the correct material queue.
				 * \exception invalid_argument The passed renderer is null
				 * \exception runtime_error Couldn't find material data for oldMat
				 * \exception runtime_error Couldn't find material data for newMat
				 */
				void materialChanged(Renderer* renderer, Material* oldMat, Material* newMat);
			protected:
				/**
				 * Gets the material data that's linked to the given material. 
				 * Returns nullptr if the material isn't known
				 * \exception invalid_argument Material is null
				 * \exception runtime_error Linked material data wasn't found
				 * \return A MaterialData object where MaterialData::material == material
				 */
				MaterialData* getMaterialData(Material* material);
				/**
				 * Recompiles the shader and updates every material and its properties
				 */
				void updateShader(ShaderFile file);
				virtual void recompileShader(ShaderFile shader) = 0;
				virtual std::unique_ptr<CameraData> createCameraData(Components::Camera* camera) = 0;
				virtual std::unique_ptr<MaterialData> createMaterialData(std::unique_ptr<Material> material) = 0;
				virtual std::unique_ptr<Material> createMaterial(std::string filePath) = 0;
				
				virtual void resizeWindow(int w, int h) = 0;

				virtual void onPreRender() = 0;
				virtual void onPostRender() = 0;

				vector<std::unique_ptr<CameraData>> cameras;
				vector<std::unique_ptr<MaterialData>> materials;
				vector<Renderer*> looseRenderers;	

				bool inPlayMode = true;

				std::unique_ptr<WindowContext> windowContext;

				GraphicsState graphicsState = INITIALIZING;
			private:
				void registerObject(Renderer* renderer);
				void deregisterObject(Renderer* renderer);
				void registerObject(Components::Camera* camera);
				void deregisterObject(Components::Camera* camera);
				virtual void render();

				bool materialsDirty = false;
			};
		}
	}
}