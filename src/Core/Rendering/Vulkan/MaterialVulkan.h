#pragma once
#include <Core/Rendering/Material.h>
#include <vulkan/vulkan.hpp>
#include "Interface/BufferVulkan.h"

namespace Tristeon
{
	namespace Data { class Image; }
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				class InternalMeshRenderer;
				class Pipeline;
				class DebugDrawManager;

				struct UniformBufferObject
				{
					glm::mat4 model, view, proj;
				};

				/**
				 * Wrapper for the Vulkan objects required to upload and use a single texture on the GPU
				 */
				struct Texture
				{
					vk::Image img = nullptr;
					vk::DeviceMemory mem = nullptr;
					vk::ImageView view = nullptr;
					vk::Sampler sampler = nullptr;
				};

				/**
				 * Vulkan specific implementation of Material.
				 * A Material controls the appearance of renderers in the world.
				 * It contains a shader, and the material's properties, such as shader parameters and textures.
				 */
				class Material : public Rendering::Material
				{
				public:
					~Material();
					Material(Pipeline* shaderPipeline, ShaderFile shader);

					/**
					 * Sets the texture property with the name [name] to the given Image.
					 * \exception invalid_argument The shader does not have a texture named [name]
					 */
					void setTexture(std::string name, std::string path) override;

					/**
					 * Sets the device memory used for the transformation data of the current object
					 */
					void bindInstanceMemory(vk::DeviceMemory const& device_memory);

					Pipeline* getPipeline() const { return shaderPipeline; }
					vk::DescriptorSet getDescriptorSet() const { return set; }
				protected:
					void resetShader() override;

					void preRenderMaterial() override;
					void postRenderMaterial() override;
					void preRenderObject(glm::mat4 model, glm::mat4 view, glm::mat4 proj) override;
					void postRenderObject() override;
					void resetResources() override;
				private:
					void saveProperty(vector<ShaderProperty> properties, std::string parentName);
					void clearResources();

					std::map<std::string, Texture> textures;
					void createTextureImage(Data::Image img, Texture& tex) const;
					void createTextureSampler(Texture& tex) const;

					Pipeline* shaderPipeline = nullptr;

					void createDescriptorSets();
					vk::DescriptorSet set;

					vk::DeviceMemory activeBufferMemory;
					BufferVulkan* createUniformBuffer(std::string name, vk::DeviceSize size);
					std::map<std::string, std::unique_ptr<BufferVulkan>> uniformBuffers;

					REGISTER_TYPE_H(Vulkan::Material)
				};
			}
		}
	}
}
