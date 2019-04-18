#pragma once

#include "glm/mat4x4.hpp"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
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

				/**
				 * Updates the view and projection matrix in GPU memory
				 */
				virtual void setViewProjection(glm::mat4 view, glm::mat4 proj) = 0;

				/**
				 * Updates the model matrix in GPU memory
				 */
				virtual void setModelMatrix(glm::mat4 model) = 0;

				std::unique_ptr<Material> material = nullptr;
				std::string materialPath = "";
				vector<Renderer*> renderers;
			};
		}
	}
}