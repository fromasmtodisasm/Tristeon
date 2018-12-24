#pragma once
#include <Core/Rendering/Components/Renderer.h>
#include <Editor/TypeRegister.h>
#include <Data/Mesh.h>
#include <XPlatform/access.h>

TRISTEON_FILE_TYPE_FORWARD(Skybox)

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			class Skybox : public Renderer
			{
				TRISTEON_FILE_TYPE(Skybox)

			public:
				Skybox();
				virtual ~Skybox() = default;

				nlohmann::json serialize() override;
				void deserialize(nlohmann::json json) override;
			protected:
				void initInternalRenderer() override;

				std::string texturePath;
				bool isDirty = true;

				Data::SubMesh mesh;
				bool cubemapLoaded = false;
			private:
				REGISTER_TYPE_H(Skybox)
			};
		}
	}
}
