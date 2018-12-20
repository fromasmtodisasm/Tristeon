﻿#include "RenderManager.h"
#include <Core/Rendering/Components/Renderer.h>
#include <Core/Message.h>
#include <Core/MessageBus.h>
#include <Core/Components/Camera.h>
#include <boost/filesystem.hpp>
#include "Graphics.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			RenderManager::RenderManager()
			{
				MessageBus::subscribeToMessage(MT_RENDER, [&](Message msg) { render(); });
				MessageBus::subscribeToMessage(MT_RENDERINGCOMPONENT_REGISTER, [&](Message msg) { registerObject(dynamic_cast<Renderer*>(msg.userData)); });
				MessageBus::subscribeToMessage(MT_RENDERINGCOMPONENT_DEREGISTER, [&](Message msg) { deregisterObject(dynamic_cast<Renderer*>(msg.userData)); });
				MessageBus::subscribeToMessage(MT_CAMERA_REGISTER, [&](Message msg) { registerObject(dynamic_cast<Components::Camera*>(msg.userData)); });
				MessageBus::subscribeToMessage(MT_CAMERA_DEREGISTER, [&](Message msg) { deregisterObject(dynamic_cast<Components::Camera*>(msg.userData)); });
				MessageBus::subscribeToMessage(MT_GAME_LOGIC_START, [&](Message msg) { inPlayMode = true; });
				MessageBus::subscribeToMessage(MT_GAME_LOGIC_STOP, [&](Message msg) { inPlayMode = false; });

				MessageBus::subscribeToMessage(MT_WINDOW_RESIZE, [&](Message msg)
				{
					Math::Vector2* vec = dynamic_cast<Math::Vector2*>(msg.userData);
					resizeWindow(static_cast<int>(vec->x), static_cast<int>(vec->y));
				});

				Graphics::renderer = this;
			}

			RenderManager::~RenderManager()
			{
				Graphics::renderer = nullptr;
			}

			MaterialData* RenderManager::getMaterialData(Material* material)
			{
				if (material == nullptr)
					throw std::invalid_argument("Trying to get material data for a null material!");

				auto const it = find_if(materials.begin(), materials.end(), [&](std::unique_ptr<MaterialData>& i) { return i->material.get() == material; });
				if (it != materials.end())
					return it->get();

				throw std::runtime_error("Couldn't find the requested material data! This shouldn't ever happen unless if the material has been obtained outside of RenderManager!");
			}

			Material* RenderManager::getMaterial(std::string filePath)
			{
				if (!exists(boost::filesystem::path(filePath)))
					return nullptr;

				auto const it = find_if(materials.begin(), materials.end(), [&](std::unique_ptr<MaterialData>& i) { return i->materialPath == filePath; });
				if (it != materials.end())
					return it->get()->material.get();

				std::unique_ptr<Material> material = move(createMaterial(filePath));
				std::unique_ptr<MaterialData> data = move(createMaterialData(move(material)));
				materials.push_back(move(data));
				materialsDirty = true;
				return material.get();
			}

			void RenderManager::updateShader(ShaderFile file)
			{
				for (int i = 0; i < materials.size(); ++i)
				{
					recompileShader(file);

					//TODO: Uncomment and friend
					//if (materials[i]->material->shaderFilePath == file.getPath())
					//{
					//	materials[i]->material->updateShader(); //Redundant?
					//	materials[i]->material->updateProperties(true); //Should be one call perhaps?
					//}
				}
			}

			void RenderManager::registerObject(Components::Camera* camera)
			{
				Misc::Console::t_assert(camera != nullptr, "Trying to register a null camera!");

				cameras.push_back(move(createCameraData(camera)));
			}

			void RenderManager::deregisterObject(Components::Camera* camera)
			{
				Misc::Console::t_assert(camera != nullptr, "Trying to deregister a null camera!");

				for (size_t i = cameras.size(); i > 0; i--)
				{
					if (cameras[i]->camera == camera)
					{
						cameras.erase(cameras.begin() + i);
						break;
					}
				}
			}

			void RenderManager::registerObject(Renderer* renderer)
			{
				Misc::Console::t_assert(renderer != nullptr, "Trying to register a null renderer!");

				if (renderer->material != nullptr)
				{
					MaterialData* material = getMaterialData(renderer->material);
					material->renderers.push_back(renderer);
				}
				else
				{
					looseRenderers.push_back(renderer);
				}

				renderer->initInternalRenderer(); //TODO: Move to Renderer::init()?
			}

			void RenderManager::deregisterObject(Renderer* renderer)
			{
				Misc::Console::t_assert(renderer != nullptr, "Trying to deregister a null renderer!");

				if (renderer->material != nullptr)
				{
					MaterialData* data = getMaterialData(renderer->material);
					data->renderers.remove(renderer);
				}
				else
				{
					looseRenderers.remove(renderer);
				}
			}

			void RenderManager::render()
			{
				onPreRender();

				if (materialsDirty)
				{
					sort(materials.begin(), materials.end(), [&](const std::unique_ptr<MaterialData>& a, const std::unique_ptr<MaterialData>& b)
					{
						return a->material->renderQueue < b->material->renderQueue;
					});
					materialsDirty = false;
				}

				for (size_t i = 0; i < cameras.size(); i++)
				{
					CameraData* camera = cameras[i].get();
					camera->onPreRender();

					//Renderers without material get renderered separately
					for(size_t j = 0; j < looseRenderers.size(); j++)
						looseRenderers[j]->render();

					for (size_t j = 0; j < materials.size(); j++)
					{
						MaterialData* material = materials[i].get();
						material->bind();
						
						for (size_t k = 0; k < material->renderers.size(); k++)
							material->renderers[i]->render(); //Renderer manages its own transform

						material->unBind();
					}

					camera->onPostRender();
				}

				onPostRender();
			}
		}
	}
}
