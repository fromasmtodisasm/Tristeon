#pragma once
#include "RenderManagerVulkan.h"
#include <vulkan/vulkan.hpp>
#include "Core/Rendering/Interface/InternalRenderer.h"
#include "Interface/BufferVulkan.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			class MeshRenderer;

			namespace Vulkan
			{
				class Forward;

				/**
				 * InternalMeshRenderer is the Vulkan implementation for the internal renderer for the meshrenderer class
				 */
				class InternalMeshRenderer : public InternalRenderer
				{
					friend Forward;
					friend RenderManager;

				public:
					explicit InternalMeshRenderer(MeshRenderer* renderer);
					~InternalMeshRenderer() override;

					/**
					 * Renders the mesh data using the allocated command buffer
					 */
					void render() override;

					/**
					* Callback function for when the mesh has been changed
					* \param mesh The new mesh
					*/
					void onMeshChange(Data::SubMesh mesh) override;
				private:
					void createUniformBuffer();
					void createDescriptorSets();

					vk::DescriptorSet set;
					MeshRenderer* meshRenderer;
					vk::CommandBuffer cmd;

					std::unique_ptr<BufferVulkan> vertexBuffer;
					std::unique_ptr<BufferVulkan> indexBuffer;
					std::unique_ptr<BufferVulkan> uniformBuffer;

					void createCommandBuffers();
					void createVertexBuffer(Data::SubMesh mesh);
					void createIndexBuffer(Data::SubMesh mesh);
				};
			}
		}
	}
}
