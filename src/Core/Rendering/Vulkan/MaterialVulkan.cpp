#include "MaterialVulkan.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Misc/Console.h"

#include "HelperClasses/CommandBuffer.h"
#include "HelperClasses/VulkanImage.h"
#include "HelperClasses/Pipeline.h"
#include "Core/BindingData.h"
#include "Misc/Hardware/Keyboard.h"
#include "RenderManagerVulkan.h"
#include "Data/ImageBatch.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				REGISTER_TYPE_CPP(Vulkan::Material)

				Material::~Material() { clearResources(); }

				void Material::render(glm::mat4 model, glm::mat4 view, glm::mat4 proj)
				{
					//TODO: This should be a separate function for recursiveness in structs and such
					//Other data


					//Reset so we don't acidentally use the buffer from last object
					uniformBufferMem = nullptr;
				}

				void Material::setupTextures()
				{
					for (const auto t : texturePaths)
					{
						Texture vktex;
						createTextureImage(Data::Image(t.second), vktex);
						createTextureSampler(vktex);
						textures[t.first] = vktex;
					}
				}

				void Material::setActiveUniformBufferMemory(vk::DeviceMemory uniformBuffer)
				{
					uniformBufferMem = uniformBuffer;
				}

				void Material::resetShader()
				{
					Rendering::Material::resetShader();
					if (shader.isEmpty)
						return;
					shaderPipeline = RenderManager::getPipeline(shader);
				}

				void Material::preRenderMaterial()
				{
					//Verify data
					if (shader.isEmpty)
					{
						Misc::Console::warning("Material.shader is undefined. Object's material properties will be off!");
						return;
					}
					if (shaderPipeline == nullptr)
					{
						Misc::Console::warning("Vulkan::Material.pipeline is nullptr. Object's material properties will be off!");
						return;
					}
				}

				void Material::saveProperty(vector<ShaderProperty> properties, std::string parentName)
				{
					for (const auto p : properties)
					{
						if (p.valueType == DT_Unknown || p.size == 0)
							continue;

						void* mem = malloc(p.size);

						switch (p.valueType)
						{
						case DT_Color:
						{
							Misc::Color const c = data.colors[parentName + "." + p.name];
							glm::vec4 col = glm::vec4(c.r, c.g, c.b, c.a);
							memcpy(mem, &col, sizeof(glm::vec4));
							break;
						}
						case DT_Float:
						{
							float f = data.floats[parentName + "." + p.name];
							memcpy(mem, &f, sizeof(float));
							break;
						}
						case DT_Vector3:
						{
							Math::Vector3 const vec = data.vectors[parentName + "." + p.name];
							glm::vec3 v = Vec_Convert3(vec);
							memcpy(mem, &v, sizeof(glm::vec3));
							break;
						}
						case DT_Struct:
						{
							uint8_t* ptr = reinterpret_cast<uint8_t*>(mem);

							saveProperty(p.children, p.name);
							for (auto c : p.children)
							{
								switch (c.valueType)
								{
								case DT_Float:
								{
									float f = data.floats[p.name + "." + c.name];
									memcpy(ptr, &f, sizeof(float));
									ptr += sizeof(float);
									break;
								}
								case DT_Color:
								{
									const auto col = colors[p.name + "." + c.name];
									glm::vec4 color = glm::vec4(col.r, col.g, col.b, col.a);
									memcpy(ptr, &color, sizeof(glm::vec4));
									ptr += sizeof(glm::vec4);
									break;
								}
								case DT_Vector3:
								{
									Math::Vector3 const vec = vectors[p.name + "." + c.name];
									glm::vec3 v = glm::vec3(vec.x, vec.y, vec.z);
									memcpy(ptr, &v, sizeof(glm::vec3));
									ptr += sizeof(glm::vec3);
									break;
								}
								}
							}
							break;
						}
						default:
						{
							free(mem);
							continue;
						}
						}

						uniformBuffers[p.name]->copyFromData(mem);
						free(mem);
					}
				}

				void Material::postRenderMaterial()
				{

				}

				void Material::preRenderObject(glm::mat4 model, glm::mat4 view, glm::mat4 proj)
				{
					if ((VkDeviceMemory)activeBufferMemory == VK_NULL_HANDLE)
					{
						Misc::Console::warning("Vulkan::Material::uniformBufferMem has not been set! Object's transform will be off!");
						return;
					}

					//Creat data
					UniformBufferObject ubo;
					ubo.model = model;
					ubo.view = view;
					ubo.proj = proj;
					ubo.proj[1][1] *= -1; //Flip handednesss due to glm being designed for OGL

					//Send data
					void* data;
					shaderPipeline->device.mapMemory(activeBufferMemory, 0, sizeof(ubo), {}, &data);
					memcpy(data, &ubo, sizeof ubo);
					shaderPipeline->device.unmapMemory(activeBufferMemory);
				}

				void Material::postRenderObject()
				{
					activeBufferMemory = nullptr;
				}

				void Material::resetResources()
				{
					if (Graphics::getGraphicsState() != IDLING)
					{
						Misc::Console::write("Trying to update material resources when the graphics state is " + std::to_string(Graphics::getGraphicsState()));
						return;
					}

					clearResources();

					if (shader.isEmpty)
						return;

					if (shaderPipeline->getShaderFile().name != shader.name)
						shaderPipeline = RenderManager::getPipeline(shader);

					if (shaderPipeline == nullptr)
						return;

					setupTextures();
					createDescriptorSets();
				}

				void Material::clearResources()
				{
					//None of this has been created if we don't have a pipeline
					if (shaderPipeline == nullptr)
						return;

					for (auto const t : textures)
					{
						VulkanBindingData::getInstance()->device.destroySampler(t.second.sampler);
						VulkanBindingData::getInstance()->device.destroyImageView(t.second.view);
						VulkanBindingData::getInstance()->device.destroyImage(t.second.img);
						VulkanBindingData::getInstance()->device.freeMemory(t.second.mem);
					}

					uniformBuffers.clear();
					textures.clear();

					shaderPipeline->device.freeDescriptorSets(VulkanBindingData::getInstance()->descriptorPool, set);
				}

				void Material::setTexture(std::string name, std::string path)
				{
					Rendering::Material::setTexture(name, path);

					//Update image if it already exists. Otherwise we can safely assume that either 
					//this texture doesn't exist, or our textures have not been set up yet
					if (textures.find(name) != textures.end())
					{
						Texture tex = textures[name];
						shaderPipeline->device.destroyImage(tex.img);
						shaderPipeline->device.freeMemory(tex.mem);
						shaderPipeline->device.destroyImageView(tex.view);
						shaderPipeline->device.destroySampler(tex.sampler);

						createTextureImage(Data::ImageBatch::getImage(path), tex);
						createTextureSampler(tex);

						textures[name] = tex;

						shaderPipeline->device.freeDescriptorSets(VulkanBindingData::getInstance()->descriptorPool, set);
						createDescriptorSets();
					}
				}

				void Material::createTextureImage(Data::Image img, Texture& texture) const
				{
					//Get image size and data
					auto const pixels = img.getPixels();
					vk::DeviceSize const size = img.getWidth() * img.getHeight() * 4;

					//Create staging buffer to allow vulkan to optimize the texture buffer's memory
					BufferVulkan staging = BufferVulkan(size, vk::BufferUsageFlagBits::eTransferSrc, 
						vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
					staging.copyFromData(pixels);

					//Create vulkan image
					VulkanImage::createImage(
						img.getWidth(), img.getHeight(),
						vk::Format::eR8G8B8A8Unorm,
						vk::ImageTiling::eOptimal,
						vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
						vk::MemoryPropertyFlagBits::eDeviceLocal,
						texture.img, texture.mem);

					//Transition to transfer destination, transfer data, transition to shader read only
					VulkanImage::transitionImageLayout(texture.img, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
					VulkanImage::copyBufferToImage(staging.getBuffer(), texture.img, img.getWidth(), img.getHeight());
					VulkanImage::transitionImageLayout(texture.img, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

					//Create image view for texture
					texture.view = VulkanImage::createImageView(shaderPipeline->device, texture.img, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
				}

				void Material::createTextureSampler(Texture& tex) const
				{
					vk::SamplerCreateInfo ci = vk::SamplerCreateInfo({},
						vk::Filter::eLinear, vk::Filter::eLinear,
						vk::SamplerMipmapMode::eLinear,
						vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
						0, VK_TRUE, 16, VK_FALSE,
						vk::CompareOp::eAlways,
						0, 0,
						vk::BorderColor::eIntOpaqueBlack,
						VK_FALSE);

					vk::Result const r = shaderPipeline->device.createSampler(&ci, nullptr, &tex.sampler);
					Misc::Console::t_assert(r == vk::Result::eSuccess, "Failed to create image sampler: " + to_string(r));
				}

				void Material::createDescriptorSets()
				{
					//Don't bother if we don't even have a shader pipeline
					if (shader == nullptr || shaderPipeline == nullptr)
						return;
					VulkanBindingData* binding = VulkanBindingData::getInstance();

					//Allocate the descriptor set we're using to pass material properties to the shader
					vk::DescriptorSetLayout layout = shaderPipeline->getSamplerLayout();
					vk::DescriptorSetAllocateInfo alloc = vk::DescriptorSetAllocateInfo(binding->descriptorPool, 1, &layout);
					vk::Result const r = binding->device.allocateDescriptorSets(&alloc, &set);
					Misc::Console::t_assert(r == vk::Result::eSuccess, "Failed to allocate descriptor set!");

					std::map<std::string, vk::DescriptorImageInfo> descImgInfos;
					std::map<std::string, vk::DescriptorBufferInfo> descBufInfos;

					std::vector<vk::WriteDescriptorSet> writes;

					//Create a descriptor write instruction for each shader property
					int i = 0;
					for (const auto pair : shader->getProps())
					{
						ShaderProperty p = pair.second;
						switch (p.valueType)
						{
							case DT_Image:
							{
								descImgInfos[p.name] = vk::DescriptorImageInfo(textures[p.name].sampler, textures[p.name].view, vk::ImageLayout::eShaderReadOnlyOptimal);
								writes.push_back(vk::WriteDescriptorSet(set, i, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descImgInfos[p.name], nullptr, nullptr));
								break;
							}
							case DT_Color:
							case DT_Float:
							case DT_Vector3:
							case DT_Struct:
							{
								BufferVulkan* const buf = createUniformBuffer(p.name, p.size);
								descBufInfos[p.name] = vk::DescriptorBufferInfo(buf->getBuffer(), 0, p.size);
								vk::WriteDescriptorSet const uboWrite = vk::WriteDescriptorSet(set, i, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descBufInfos[p.name], nullptr);
								writes.push_back(uboWrite);
								break;
							}
						}
						i++;
					}

					//Update descriptor with our new write info
					binding->device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
				}

				BufferVulkan* Material::createUniformBuffer(std::string name, vk::DeviceSize size)
				{
					//Create a uniform buffer of the size of UniformBufferObject
					if (uniformBuffers.find(name) != uniformBuffers.end())
						return uniformBuffers[name].get();

					VulkanBindingData* bind = VulkanBindingData::getInstance();
					uniformBuffers[name] = std::make_unique<BufferVulkan>(bind->device, bind->physicalDevice, bind->swapchain->getSurface(), size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
					return uniformBuffers[name].get();
				}
			}
		}
	}
}