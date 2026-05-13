#pragma once
#include "src/compiler/frontend/FrontendPipeline.h"
#include "src/utils/console/CommandLineParser.h"

namespace BackendPipeline
{
bool Run(FrontendPipline::FrontendResult& result, const CompilerOptions& options);
}
