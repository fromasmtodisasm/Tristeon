#include "ShaderParser.h"
#include <ShaderConductor/ShaderConductor.hpp>

namespace Tristeon
{
	namespace Core
	{
		namespace Rendering
		{
			ShaderContents ShaderParser::CompileVulkanSVP(ShaderContents contents)
			{
				ShaderConductor::Blob* blob = ShaderConductor::CreateBlob(contents.vertex.c_str(), contents.vertex.size());

				ShaderConductor::Compiler::Options options;
				options.optimizationLevel = 3;
				options.disableOptimizations = false;

				ShaderConductor::Compiler::SourceDesc source;

				ShaderConductor::Compiler::ResultDesc result = ShaderConductor::Compiler::Compile();

				ShaderConductor::DestroyBlob(blob);
			}
		}
	}
}