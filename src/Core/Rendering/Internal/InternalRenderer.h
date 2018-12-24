#pragma once
#include "Core/TObject.h"
#include "Data/Mesh.h"

namespace Tristeon {
	namespace Data {
		struct SubMesh;
	}
}

namespace Tristeon
{
	namespace Core
	{
		class BindingData;

		namespace Rendering
		{
			class Renderer;

			/**
			 * The internal renderer is an interface that is used to run 
			 * API specific draw calls without interfering with the automatically serialized component system.
			 */
			class InternalRenderer : public TObject
			{
			public:
				explicit InternalRenderer(Renderer* pOwner);
				virtual void render() = 0;
				virtual void onMeshChange(Data::SubMesh mesh) {}
			private:
				Renderer* owner;
			};
		}
	}
}
