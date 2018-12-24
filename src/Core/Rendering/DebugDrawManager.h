#pragma once
#include "Data/Mesh.h"
#include "Misc/Color.h"
#include <queue>

namespace Tristeon
{
	//Forward decl
	namespace Math { struct Vector3; }

	namespace Core
	{
		namespace Rendering
		{
			/**
			 * DebugDrawManager is a utility class for drawing primitive shapes. This class gets inherited to define API specific rendering behavior
			 */
			class DebugDrawManager
			{
			public:
				/**
				 * Adds a line to the drawlist
				 * \param from The starting point of the line
				 * \param to The end point of the line
				 * \param width The width of the line 
				 * \param color The color of the line
				 */
				static void addLine(const Math::Vector3& from, const Math::Vector3& to, float width, const Misc::Color& color);
				/**
				 * Adds a cube to the drawlist
				 * \param min The smallest point of the cube
				 * \param max The biggest point of a cube
				 * \param lineWidth The width of the lines
				 * \param color The color of the cube
				 */
				static void addCube(const Math::Vector3& min, const Math::Vector3& max, float lineWidth, const Misc::Color& color);
				/**
				 * Adds a sphere to the drawlist
				 * \param center The center position of the sphere
				 * \param radius The radius of the sphere
				 * \param lineWidth The width of the lines of the sphere
				 * \param color The color of the sphere
				 * \param circles The amount of horizontal and vertical circles used to display the sphere
				 * \param resolution The amount of points in the sphere
				 */
				static void addSphere(const Math::Vector3& center, float radius, float lineWidth, const Misc::Color& color, int circles = 4, int resolution = 15);
				
				virtual ~DebugDrawManager() = default;
			protected:
				/**
				 * The line struct describes a single renderable line, with a start, end, width and color defining its appearance and position.
				 */
				struct Line
				{
					Data::Vertex start;
					Data::Vertex end;
					float width;
					Misc::Color color;
					
					Line(Data::Vertex start, Data::Vertex end, float width, Misc::Color color) : start(start), end(end), width(width), color(color) { /*Empty*/ }
				};

				static DebugDrawManager* instance;
				std::queue<Line> drawList;
				virtual void draw() = 0;
			};
		}
	}
}