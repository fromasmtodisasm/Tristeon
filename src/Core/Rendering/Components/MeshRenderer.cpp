﻿#include "MeshRenderer.h"
#include <Core/UserPrefs.h>
#include <Misc/Console.h>
#include <Core/Rendering/Vulkan/InternalMeshRendererVulkan.h>
#include <Data/MeshBatch.h>
#include <XPlatform/typename.h>

#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			REGISTER_TYPE_CPP(MeshRenderer)

			void MeshRenderer::initInternalRenderer()
			{
				if (UserPrefs::getStringValue("RENDERAPI") == "VULKAN")
					_internalRenderer = std::make_unique<Vulkan::InternalMeshRenderer>(this);
				else
					Misc::Console::error("Trying to create a MeshRenderer with unsupported rendering API");
			}

			nlohmann::json MeshRenderer::serialize()
			{
				nlohmann::json j = Renderer::serialize();
				j["typeID"] = TRISTEON_TYPENAME(MeshRenderer);
				j["name"] = "MeshRenderer";
				j["meshPath"] = meshFilePath;
				j["subMeshID"] = subMeshID;
				return j;
			}

			void MeshRenderer::deserialize(nlohmann::json json)
			{
				Renderer::deserialize(json);

				const std::string meshFilePathValue = json["meshPath"];
				const unsigned int submeshIDValue = json["subMeshID"];

				if (meshFilePath != meshFilePathValue || subMeshID != submeshIDValue)
				{
					if (filesystem::exists(meshFilePathValue))
						mesh = Data::MeshBatch::getSubMesh(meshFilePathValue, submeshIDValue);
					else
						mesh = Data::SubMesh();
				}

				meshFilePath = meshFilePathValue;
				subMeshID = submeshIDValue;
			}
		}
	}
}
