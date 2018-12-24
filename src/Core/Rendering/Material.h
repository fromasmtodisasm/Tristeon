#pragma once
#include <Core/TObject.h>
#include <Editor/TypeRegister.h>

#include <Math/Vector3.h>
#include <glm/mat4x4.hpp>

#include <Misc/Color.h>
#include <Misc/Property.h>

#include <Core/Rendering/ShaderFile.h>
#include <Core/Rendering/Graphics.h>

#ifdef TRISTEON_EDITOR
namespace Tristeon {
	namespace Editor {
		class MaterialFileItem;
	}
}
#endif

namespace Tristeon
{
	namespace Core
	{
		namespace Components { class Camera; }

		namespace Rendering
		{
			namespace Vulkan { class RenderManager; }
			class RenderManager;

			/**
			 * A Material controls the appearance of renderers in the world.
			 * It contains a shader, and the material's properties, such as shader parameters and textures.
			 */
			class Material : public TObject
			{
				friend Vulkan::RenderManager;
				friend RenderManager;
#ifdef TRISTEON_EDITOR
				friend Editor::MaterialFileItem;
#endif
			public:

				/**
				 * Sets the texture property with the name [name] to the given Image.
				 * \exception invalid_argument The shader does not have a texture named [name]
				 */
				virtual void setTexture(std::string name, std::string path);
				/**
				 * Sets the float property with the name [name] to the given value.
				 * \exception invalid_argument The shader does not have a variable named [name] 
				 */
				virtual void setFloat(std::string name, float value);
				/**
				 * Sets the vector3 property with the name [name] to the given vector.
				 * \exception invalid_argument The shader does not have a variable named [name] 
				 */
				virtual void setVector3(std::string name, Math::Vector3 value);
				/**
				 * Sets the color property with the name [name] to the given color. These are Vector4s in the shader
				 * \exception invalid_argument The shader does not have a variable named [name]
				 */
				virtual void setColor(std::string name, Misc::Color value);

				Property(Material, renderQueue, int);
				GetProperty(renderQueue) { return renderqueue; }
				SetProperty(renderQueue) { renderQueue = value; Graphics::setMaterialsDirty(); } 

				nlohmann::json serialize() override;
				void deserialize(nlohmann::json json) override;
			protected:
				/**
				 * OnRender function, override this function to send data to the shader. API specific.
				 * \param model The transformation matrix of the object
				 * \param view The current camera view matrix
				 * \param proj The current camera projection matrix
				 */
				virtual void render(glm::mat4 model, glm::mat4 view, glm::mat4 proj) { }

				/**
				 * Used to rebuild the shader pipeline and update our properties when we've changed ShaderFile
				 */
				virtual void updateProperties(bool updateResources = true) { }

				virtual void updateShader();

				std::unique_ptr<ShaderFile> shader;
				std::map<std::string, std::string> texturePaths;
				std::map<std::string, Math::Vector3> vectors;
				std::map<std::string, Misc::Color> colors;
				std::map<std::string, float> floats;
			private:
				void checkProperty(std::string parentName, ShaderProperty prop, const std::map<std::string, std::string>& tex, const std::map<std::string, Math::Vector3>& vec, const std::map<std::string, Misc::Color>& col, const std::map<std::string, float>& fl);
				std::string shaderFilePath;
				int renderqueue = 0;

				REGISTER_TYPE_H(Material)
			};
		}
	}
}
