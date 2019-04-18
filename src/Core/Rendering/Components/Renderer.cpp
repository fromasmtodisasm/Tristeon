#include "Renderer.h"
#include <Core/Message.h>
#include <Core/MessageBus.h>
#include "Core/Rendering/Interface/InternalRenderer.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			void Renderer::render()
			{
				if (_internalRenderer != nullptr)
					_internalRenderer->render();
			}

			Renderer::~Renderer()
			{
				if (registered)
					MessageBus::sendMessage({ MT_RENDERINGCOMPONENT_DEREGISTER, dynamic_cast<TObject*>(this) });
			}

			nlohmann::json Renderer::serialize()
			{
				nlohmann::json j;
				j["materialPath"] = materialPath;
				return j;
			}

			void Renderer::deserialize(nlohmann::json json)
			{
				const std::string materialPathValue = json["materialPath"];
				if (materialPath != materialPathValue)
				{
					material = Graphics::getMaterial(materialPathValue);
					materialPath = materialPathValue;
				}
			}

			void Renderer::onResize() const
			{
				_internalRenderer->onResize();
			}

			void Renderer::init()
			{
				if (!registered)
					MessageBus::sendMessage({MT_RENDERINGCOMPONENT_REGISTER, dynamic_cast<TObject*>(this) });
				registered = true;
			}
		}
	}
}
