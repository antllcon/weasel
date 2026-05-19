#pragma once
#include "src/compiler/frontend/FrontendPipeline.h"
#include "src/utils/console/CommandLineParser.h"

namespace BackendPipeline
{
bool Run(FrontendPipeline::FrontendResult& result, const CompilerOptions& options);
}
