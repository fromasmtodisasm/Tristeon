#pragma once
#include "Renderer.h"
#include <Data/Mesh.h>
#include <Misc/Property.h>
#include <Editor/TypeRegister.h>
#include <Core/Rendering/Internal/InternalRenderer.h>

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			/**
			 * The MeshRenderer is a specification of Renderer.
			 * It inherits all the features of Renderer, 
			 * except it specifically displays a mesh in 3D space.
			 */
			class MeshRenderer : public Renderer
			{
			public:
				Property(MeshRenderer, mesh, Data::SubMesh);
				SetProperty(mesh)
				{
					_mesh = value;
					if (_internalRenderer != nullptr)
						_internalRenderer->onMeshChange(value);
				}
				GetProperty(mesh) { return _mesh; }
				
				void initInternalRenderer() override;

				nlohmann::json serialize() override;
				void deserialize(nlohmann::json json) override;
			private:
				Data::SubMesh _mesh;
				std::string meshFilePath = "";
				uint32_t subMeshID = 0;

				REGISTER_TYPE_H(MeshRenderer)
			};
		}
	}
}
