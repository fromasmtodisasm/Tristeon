#pragma once
#include "Core/Components/Component.h"
#include <Core/Rendering/Graphics.h>

namespace Tristeon
{
	namespace Core
	{
		class BindingData;
		class MessageBus;
		namespace Rendering
		{
			class InternalRenderer;
			class RenderManager;
			class Material;

			/**
			 * Renderer is the base class for all renderers in the engine.
			 * It allows for objects to be displayed in-game/editor. 
			 * It provides base functionality and wraps around a so called "InternalRenderer", 
			 * which is designed to be created based on the current selected rendering API.
			 */
			class Renderer : public Components::Component
			{
				friend RenderManager;
			public:
				~Renderer();
				/**
				 * Render is the main rendering function of the renderer and calls internalRenderer.render(). It can be overriden if needed.
				 */
				virtual void render();

				/**
				* The (shared) material of the renderer. 
				* Changing any properties of this material will also change the properties of any other renderer with this material.
				*/
				Property(Renderer, material, Material*);
				GetProperty(material) { return _material; }
				SetProperty(material) 
				{ 
					Material* old = _material; 
					_material = value; 
					Graphics::materialChanged(this, old, value); 
				}

				/**
				 * The internal renderer. Contains rendering API specific calls / data.
				 */
				ReadOnlyProperty(Renderer, internalRenderer, InternalRenderer*)
				GetProperty(internalRenderer) { return _internalRenderer.get(); }

				nlohmann::json serialize() override;
				void deserialize(nlohmann::json json) override;
			protected:
				void init() override;
				virtual void initInternalRenderer() = 0;

				std::unique_ptr<InternalRenderer> _internalRenderer;
				std::string materialPath;
				Material* _material = nullptr;
				bool registered = false;
			};
		}
	}
}
