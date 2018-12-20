#pragma once
#include <Misc/vector.h>
#include <Core/Rendering/ShaderFile.h>
#include <Core/TObject.h>

namespace Tristeon
{
	namespace Core
	{
		namespace Components { class Camera; }
		namespace Rendering
		{
			class Renderer; 
			class Material;

			/**
			 * Base class for containing / using camera data and logic. 
			 * Intended to be inherited by API specific classes.
			 */
			class CameraData : public TObject
			{
			public:
				CameraData() = default;
				virtual ~CameraData() = default;
				explicit CameraData(Components::Camera* pCamera) : camera(pCamera) { }

				/**
				 * Called before the scene is rendered. Used to prepare the API for rendering, and to pass camera data to the GPU.
				 */
				virtual void onPreRender() = 0;
				/**
				 * Called after the scene is rendered. Used to allow the API to finish anything related to the camera
				 */
				virtual void onPostRender() = 0;

				Components::Camera* camera = nullptr;
			};

			/**
			 * Base class for containing / using material data and logic.
			 * Intended to be inherited by API specific classes
			 */
			class MaterialData : public TObject
			{
			public:
				MaterialData() = default;
				virtual ~MaterialData() = default;
				explicit MaterialData(std::unique_ptr<Material> pMaterial) {
					material = move(pMaterial);
				}

				/**
				 * Called before objects with this material are rendered. Used to send material data (textures, parameters, etc) to the GPU.
				 */
				virtual void bind() = 0;
				/**
				 * Called after the objects with this material have been rendered. Used to allow the material to clean up any data / renderstate it might've set.
				 */
				virtual void unBind() = 0;

				std::unique_ptr<Material> material = nullptr;
				std::string materialPath = "";
				vector<Renderer*> renderers;
			};

			class RenderManager
			{
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

				void setMaterialsDirty() { materialsDirty = true; }
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
			private:
				void registerObject(Renderer* renderer);
				void deregisterObject(Renderer* renderer);
				void registerObject(Components::Camera* camera);
				void deregisterObject(Components::Camera* camera);
				void render();

				bool materialsDirty = false;
			};
		}
	}
}