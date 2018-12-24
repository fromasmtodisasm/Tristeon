#pragma once

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			class Window;

			/**
			 * WindowContext is an interface for interacting with the Rendering API's window functionality.
			 * It defines functions for the preparation and finalisation of a frame, and it manages window resizing.
			 * Each rendering API will inherit this class to implement its own window handling specific code
			 */
			class WindowContext
			{
			public:
				explicit WindowContext(Window* window);

				/**
				 * Prepares the next frame for rendering purposes.
				 */
				virtual void prepareFrame() = 0;
				/**
				 * Finishes the current frame and instructs the GPU to display it if necessary.
				 */
				virtual void finishFrame() = 0;
				/**
				 * Resizes its resources to the correct window size.
				 */
				virtual void resize(int width, int height) = 0;
			protected:
				Window* window = nullptr;
			};
		}
	}
}