#include "RenderManagerVulkan.h"

//Engine 
#include <Core/BindingData.h>
#include "Core/Message.h"
#include "Core/UserPrefs.h"
#include "Misc/Console.h"
#include "Math/Vector2.h"
#include "Core/Transform.h"

#include "Core/MessageBus.h"
#include "Core/Rendering/Components/Renderer.h"
#include "InternalMeshRendererVulkan.h"

//Vulkan 
#include "HelperClasses/QueueFamilyIndices.h"
#include "HelperClasses/Swapchain.h"
#include "HelperClasses/Pipeline.h"
#include "HelperClasses/EditorGrid.h"
#include "HelperClasses/CameraRenderData.h"
#include "MaterialVulkan.h"
#include <Core/Rendering/Material.h>

//RenderTechniques
#include "ForwardVulkan.h"

//Vulkan help classes
#include "API/Extensions/VulkanExtensions.h"
#include "HelperClasses/VulkanFormat.h"
#include "Editor/JsonSerializer.h"
#include "DebugDrawManagerVulkan.h"
#include "SkyboxVulkan.h"
#include "../Skybox.h"
#include "Misc/ObjectPool.h"
#include "Core/GameObject.h"
#include "API/WindowContextVulkan.h"

#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;

using Tristeon::Misc::Console;

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			namespace Vulkan
			{
				RenderManager::RenderManager() : window(BindingData::getInstance()->window)
				{
					MessageBus::subscribeToMessage(MT_WINDOW_RESIZE, [&](Message msg)
					{
						Math::Vector2* vec = reinterpret_cast<Math::Vector2*>(msg.userData);
						resizeWindow(static_cast<int>(vec->x), static_cast<int>(vec->y));
					});

#ifdef TRISTEON_EDITOR
					MessageBus::subscribeToMessage(MT_PRERENDER, [&](Message msg) { MessageBus::sendMessage({ MT_SHARE_DATA, &editor }); });
#endif

					//Create render technique
					const std::string tech = UserPrefs::getStringValue("RENDERTECHNIQUE");
					if (tech == "FORWARD")
						technique = new Forward(this);
					else
						Console::error("Trying to use an unsupported Render technique: " + tech);

					//Setup vulkan
					setupVulkan();

#ifdef TRISTEON_EDITOR
					//Setup editor camera
					editor.trans = new Transform();
					editor.cam = new CameraRenderData();
					editor.cam->init(this, offscreenPass, onscreenPipeline, true);

					grid = new EditorGrid(offscreenPass);
					editorSkybox = (Vulkan::Skybox*)getSkybox("Files/Misc/SkyboxEditor.skybox");
#endif
					//Create the debug draw manager
					DebugDrawManager::instance = new Vulkan::DebugDrawManager(offscreenPass);
				}

				void RenderManager::render()
				{
					//TODO: This should be done in the base class. Leave room for customization tho

					//Don't render anything if there's nothing to render
					if (internalRenderers.size() == 0 && renderables.size() == 0)
						return;
					
					windowContext->preRenderFrame();

					//Render scene
					renderScene();

					//Render cameras
					technique->renderCameras();

					//Submit frame
					submitCameras();

					windowContext->postRenderFrame();
				}

				Pipeline* RenderManager::getPipeline(ShaderFile file)
				{
					RenderManager* rm = (RenderManager*)instance;

					for (Pipeline* p : rm->pipelines)
						if (p->getShaderFile().getNameID() == file.getNameID())
							return p;

					Pipeline *p = new Pipeline(file, 
						rm->vkContext->getSwapchain()->extent2D.get(), 
						rm->offscreenPass, 
						true, 
						vk::PrimitiveTopology::eTriangleList, 
						false, 
						vk::CullModeFlagBits::eBack, 
						file.hasVariable(2, 0, DT_Image, ST_Fragment));
					rm->pipelines.push_back(p);
					return p;
				}

				RenderManager::~RenderManager()
				{
					//Wait till the device is finished
					vk::Device d = vkContext->getDevice();
					d.waitIdle();

					delete DebugDrawManager::instance;

					//Render technique
					delete technique;
					for (auto i : internalRenderers) delete i;
					internalRenderers.clear();

					//Cameras and renderpasses
					cameraDataPool.reset();
					d.destroyRenderPass(offscreenPass);

					skyboxes.clear();
#ifdef TRISTEON_EDITOR
					//Editor
					delete editor.trans;
					delete grid;
					delete editor.cam;
#endif
					//Materials
					for (auto m : materials) delete m.second;
					for (Pipeline* p : pipelines) delete p;
					delete onscreenPipeline;
					
					//DescriptorPool
					d.destroyDescriptorPool(descriptorPool);
					
					//Commandpool
					d.destroyCommandPool(commandPool);

					//Core
					vkContext = nullptr;
					windowContext.reset();
				}

				void RenderManager::setupVulkan()
				{
					//Core vulkan
					vkContext = new WindowContextVulkan(reinterpret_cast<Window*>(BindingData::getInstance()->tristeonWindow));
					windowContext = std::unique_ptr<WindowContextVulkan>(vkContext);
					
					VulkanBindingData* bindingData = VulkanBindingData::getInstance();
					bindingData->physicalDevice = vkContext->getGPU();
					bindingData->device = vkContext->getDevice();
					bindingData->graphicsQueue = vkContext->getGraphicsQueue();
					bindingData->presentQueue = vkContext->getPresentQueue();

					bindingData->swapchain = vkContext->getSwapchain();
					bindingData->renderPass = vkContext->getSwapchain()->renderpass;

					//Pools
					createDescriptorPool();
					createCommandPool();

					//Offscreen/onscreen data
					prepareOffscreenPass();
					prepareOnscreenPipeline();

					//Create main screen framebuffers
					vkContext->getSwapchain()->createFramebuffers();

					//Create main screen command buffers
					createCommandBuffer();

					//Initialize textures and descriptorsets
					for (auto m : materials)
						m.second->updateProperties();
				}

				Rendering::Skybox* RenderManager::_getSkybox(std::string filePath)
				{
					if (filePath == "")
						return nullptr;
					if (!filesystem::exists(filePath))
						return nullptr;
					if (filesystem::path(filePath).extension() != ".skybox")
						return nullptr;

					Skybox* skybox = new Skybox(offscreenPass);
					skybox->deserialize(JsonSerializer::load(filePath));
					skybox->init();
					skyboxes[filePath] = std::unique_ptr<Rendering::Skybox>(skybox);
					return skyboxes[filePath].get();
				}

				void RenderManager::_recompileShader(std::string filePath)
				{
					ShaderFile* file = JsonSerializer::deserialize<ShaderFile>(filePath);
					Pipeline* pipeline = getPipeline(*file);
					pipeline->recompile(*file);

					for (const auto mat : materials)
					{
						if (mat.second->shaderFilePath == filePath)
						{
							mat.second->updateShader();
							mat.second->updateProperties(true);
						}
					}
				}

				void RenderManager::resizeWindow(int newWidth, int newHeight)
				{
					//Don't resize if the size is invalid
					if (newWidth <= 0 || newHeight <= 0)
						return;

					windowContext->resize(newWidth, newHeight);
					VulkanBindingData::getInstance()->renderPass = vkContext->getRenderpass();

					//Rebuild offscreen renderpass
					vkContext->getDevice().destroyRenderPass(offscreenPass);
					prepareOffscreenPass();

					//Rebuild camera data
					for (auto const p : cameraData)
						p.second->rebuild(this, offscreenPass, onscreenPipeline);
#ifdef TRISTEON_EDITOR
					//Rebuild editor data
					if (editor.cam != nullptr)
						editor.cam->rebuild(this, offscreenPass, onscreenPipeline);
					grid->rebuild(offscreenPass);
					editorSkybox->rebuild(vkContext->getExtent(), offscreenPass);
#endif
					//Rebuild pipelines
					for (int i = 0; i < pipelines.size(); i++)
						pipelines[i]->onResize(vkContext->getExtent(), offscreenPass);

					((DebugDrawManager*)DebugDrawManager::instance)->rebuild(offscreenPass);

					//Rebuild framebuffers
					vkContext->getSwapchain()->createFramebuffers();

					const auto end = skyboxes.end();
					for (auto i = skyboxes.begin(); i != end; ++i)
						((Skybox*)i->second.get())->rebuild(vkContext->getExtent(), offscreenPass);
				}
			}
		}
	}
}