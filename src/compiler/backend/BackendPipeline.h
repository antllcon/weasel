#pragma once
#include "src/compiler/frontend/FrontendPipeline.h"
#include "src/utils/console/CommandLineParser.h"

namespace BackendPipeline
{
void Run(FrontendPipline::FrontendResult& result, const CompilerOptions& options);
}
