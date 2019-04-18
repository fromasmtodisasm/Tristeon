#pragma once
#include <string>

using std::string;

namespace Tristeon
{
	namespace Core 
	{
		namespace Rendering
		{
			struct ShaderContents
			{
				string vertex;
				string fragment;
			};

			class ShaderParser
			{
			public:
				ShaderContents CompileVulkanSVP(ShaderContents contents);
			};
		}
	}
}