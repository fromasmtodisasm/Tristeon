#include "Material.h"
#include "Editor/JsonSerializer.h"
#include "XPlatform/typename.h"

#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;

#define SERIALIZE_MAP(jsonName, keyType, valueType, map) for (auto& element : nlohmann::json::iterator_wrapper(jsonName)) \
	{\
		const keyType key = element.key(); \
		const valueType value = element.value(); \
		map[key] = value; \
	}

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			REGISTER_TYPE_CPP(Material)

			nlohmann::json Material::serialize()
			{
				nlohmann::json j;
				j["typeID"] = TRISTEON_TYPENAME(Material);
				j["texturePaths"] = data.texturePaths;
				j["vectors"] = data.vectors;
				j["colors"] = data.colors;
				j["floats"] = data.floats;
				j["shaderFilePath"] = shaderFilePath;
				return j;
			}

			void Material::deserialize(nlohmann::json json)
			{
				//Get the shader file
				const std::string shaderFilePathValue = json["shaderFilePath"];
				//Only update our shader if our path has changed
				if (shaderFilePath != shaderFilePathValue)
				{
					shaderFilePath = shaderFilePathValue;
					resetShader();
				}

				//If we don't have a shader there's also no material property data to deserialize
				//(or at least we'll just assume so)
				if (shader.isEmpty)
					return;

				MaterialData serializedData{};
				SERIALIZE_MAP(json["texturePaths"], std::string, std::string, serializedData.texturePaths);
				SERIALIZE_MAP(json["vectors"], std::string, Math::Vector3, serializedData.vectors);
				SERIALIZE_MAP(json["colors"], std::string, Misc::Color, serializedData.colors);
				SERIALIZE_MAP(json["floats"], std::string, float, serializedData.floats);
				resetProperties(serializedData);
			}

			void Material::setTexture(std::string name, std::string path)
			{
				if (data.texturePaths.find(name) != data.texturePaths.end())
					data.texturePaths[name] = path;
				else
					throw std::invalid_argument("Trying to set material Texture [" + name + "] but that name is not linked to a texture!");
			}

			void Material::setFloat(std::string name, float value)
			{
				if (data.floats.find(name) != data.floats.end())
					data.floats[name] = value;
				else
					throw std::invalid_argument("Trying to set material float [" + name + "] but that name is not linked to a valid variable!");
			}

			void Material::setVector3(std::string name, Math::Vector3 value)
			{
				if (data.vectors.find(name) != data.vectors.end())
					data.vectors[name] = value;
				else
					throw std::invalid_argument("Trying to set material Vector3 [" + name + "] but that name is not linked to a valid variable!");
			}

			void Material::setColor(std::string name, Misc::Color value)
			{
				if (data.colors.find(name) != data.colors.end())
					data.colors[name] = value;
				else
					throw std::invalid_argument("Trying to set material Color [" + name + "] but that name is not linked to a valid variable!");
			}

			void Material::resetShader()
			{
				if (filesystem::exists(shaderFilePath) && filesystem::path(shaderFilePath).extension() == ".shader")
				{
					ShaderFile* newShader = JsonSerializer::deserialize<ShaderFile>(shaderFilePath);
					if (newShader == nullptr)
					{
						shader = ShaderFile();
						return;
					}

					shader = *newShader;
					delete newShader;
				}
			}

			void Material::resetProperties(MaterialData serializedData)
			{
				data.clear();

				if (!shader.isEmpty)
					registerProperties("", shader.getProperties(), serializedData);

				resetResources();
			}

			void Material::registerProperties(std::string parentName, vector<ShaderProperty> properties, MaterialData serializedData)
			{
				#define VALIDATE(map, def) map.find(name) != map.end() ? map.at(name) : def;

				for (auto const p : properties)
				{
					std::string const name = parentName + p.name;
					switch (p.valueType)
					{
						case DT_Image: this->data.texturePaths[name] = VALIDATE(serializedData.texturePaths, ""); break;
						case DT_Color: this->data.colors[name] = VALIDATE(serializedData.colors, Misc::Color()); break;
						case DT_Float: this->data.floats[name] = VALIDATE(serializedData.floats, 0); break;
						case DT_Vector3: this->data.vectors[name] = VALIDATE(serializedData.vectors, Math::Vector3()); break;
						case DT_Struct: registerProperties(name + ".", p.children, serializedData); break;
						default: break;
					}
				}
			}
		}
	}
}