#include "ShaderFile.h" 
#include <Misc/Console.h>
#include <Core/UserPrefs.h>
#include <XPlatform/typename.h>
#include <Core/Rendering/Graphics.h>

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			REGISTER_TYPE_CPP(ShaderFile)

			ShaderFile::ShaderFile(std::string name, std::string directory, std::string vertexName, std::string fragmentName)
				: nameID(name), directory(directory), vertexName(vertexName), fragmentName(fragmentName)
			{
				//Empty
			}

			std::string ShaderFile::getFilePath(std::string api, ShaderStage stage) const
			{
				//Get the name and the shader file extension of the api
				std::string apiName, apiExtension;
				if (api == "VULKAN")
				{
					apiName = "Vulkan";
					apiExtension = ".spv";
				}
				else
					Misc::Console::error("Trying to create vertex shader with unsupported Graphics API!");

				//Get the name of the file based on the shader type
				std::string fileName;
				switch (stage)
				{
				case Vertex:
					fileName = vertexName;
					break;
				case Fragment:
					fileName = fragmentName;
					break;
				default:
					Misc::Console::error("Unsupported shader stage!");
				}

				//Result
				return directory + fileName + apiName + apiExtension;
			}

			nlohmann::json ShaderFile::serialize()
			{
				nlohmann::json j;
				j["typeID"] = TRISTEON_TYPENAME(ShaderFile);
				j["nameID"] = nameID;
				j["directory"] = directory;
				j["vertexName"] = vertexName;
				j["fragmentName"] = fragmentName;
				return j;
			}

			void ShaderFile::deserialize(nlohmann::json json)
			{
				const std::string tempNameID = json["nameID"];
				nameID = tempNameID;

				const std::string tempDirectory = json["directory"];
				directory = tempDirectory;

				const std::string tempVertexName = json["vertexName"];
				vertexName = tempVertexName;

				const std::string tempFragmentName = json["fragmentName"];
				fragmentName = tempFragmentName;
			}

			vector<ShaderProperty> ShaderFile::getProperties()
			{
				if (!loadedProps)
				{
					Graphics::reflectShader(*this);
					loadedProps = true;
				}
				return properties;
			}
		}
	}
}
