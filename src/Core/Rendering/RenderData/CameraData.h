#pragma once
#include "Core/Components/Camera.h"

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			/**
			* Base class for containing / using camera data and logic.
			* Intended to be inherited by API specific classes.
			*/
			class CameraData : public TObject
			{
			public:
				CameraData() = default;
				virtual ~CameraData() = default;
				explicit CameraData(Components::Camera* pCamera) : camera(pCamera) { }

				/**
				 * Called before the scene is rendered. Used to prepare the API for rendering, and to pass camera data to the GPU.
				 */
				virtual void onPreRender() = 0;
				/**
				 * Called after the scene is rendered. Used to allow the API to finish anything related to the camera
				 */
				virtual void onPostRender() = 0;

				/**
				 * Rebuilds rendering resources.
				 */
				virtual void onResize() = 0;

				Components::Camera* camera = nullptr;
			};
		}
	}
}