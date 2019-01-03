#pragma once
#include <string>
#include <Misc/Property.h>

#include <Editor/Serializable.h>
#include <Editor/TypeRegister.h>
#include <Core/UserPrefs.h>
#include <Misc/vector.h>

#ifdef TRISTEON_EDITOR
namespace Tristeon {
	namespace Editor {
		class ShaderFileItem;
	}
}
#endif

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			enum DataType
			{
				DT_Unknown,
				DT_Image,
				DT_Color,
				DT_Float,
				DT_Vector3,
				DT_Struct
			};

			enum ShaderStage
			{
				Vertex,
				Fragment,
				Geometry,
				Compute,
				All_Graphics,
				All
			};

			class PropertyAPIData { };
			/**
			 * A description of a runtime changeable property in a shader.
			 */
			struct ShaderProperty
			{
				std::string name;
				DataType valueType;
				ShaderStage shaderStage;
				size_t size;
				vector<ShaderProperty> children;
				std::shared_ptr<PropertyAPIData> apiData; 

				bool operator ==(const ShaderProperty& other) const
				{
					return other.name == name && other.valueType == valueType && other.shaderStage == shaderStage;
				}
			};

			/**
			 * ShaderFile is a class that is used to describe a shader's path and its properties.
			 */
			class ShaderFile final : public Serializable
			{
#ifdef TRISTEON_EDITOR
				friend Editor::ShaderFileItem;
#endif
			public:
				/**
				 * Creates an empty, invalid shaderfile. Can be used as a placeholder.
				 */
				ShaderFile() : ShaderFile("", "", "", "") { }

				ShaderFile(const ShaderFile& other) = default;
				void operator =(const ShaderFile& other) { *this = ShaderFile(other); }

				/**
				 * Creates a new shaderfile with the given data. Used for custom non-serialized shader files.
				 * \param name The name ID of the shader.
				 * \param directory The directory of the shader files.
				 * \param vertexName The name of the vertex shader.
				 * \param fragmentName The name of the fragment shader.
				 */
				ShaderFile(std::string name, std::string directory, std::string vertexName, std::string fragmentName);

				ReadOnlyProperty(ShaderFile, vertexFilePath, std::string)
				GetProperty(vertexFilePath) { return getFilePath(UserPrefs::getStringValue("RENDERAPI"), Vertex); }

				ReadOnlyProperty(ShaderFile, fragmentFilePath, std::string)
				GetProperty(fragmentFilePath) { return getFilePath(UserPrefs::getStringValue("RENDERAPI"), Fragment); }

				ReadOnlyProperty(ShaderFile, name, std::string)
				GetProperty(name) { return nameID;}

				ReadOnlyProperty(ShaderFile, isEmpty, bool)
				GetProperty(isEmpty) { return nameID == "" && directory == "" && vertexName == "" && fragmentName == ""; }

				nlohmann::json serialize() override;
				void deserialize(nlohmann::json json) override;

				/**
				 * Gets all the properties in all the added shader stages.
				 * Returns an empty vector if the compilation fails.
				 */
				vector<ShaderProperty> getProperties();

				bool operator ==(const ShaderFile& other);
				bool operator !=(const ShaderFile& other) { return !operator==(other); }
			private:
				std::string getFilePath(std::string api, ShaderStage stage) const;
				
				std::string nameID;
				std::string directory;
				std::string vertexName;
				std::string fragmentName;
				
				bool loadedProps = false;
				vector<ShaderProperty> properties;

				REGISTER_TYPE_H(ShaderFile)
			};
		}
	}
}