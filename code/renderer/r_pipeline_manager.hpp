#pragma once

#include "asset/types/pipeline.hpp"

namespace PG::Gfx::PipelineManager
{

void Init();
void Shutdown();

Pipeline CreatePipeline( const PipelineCreateInfo& createInfo );
Pipeline* GetPipeline( const std::string& name, bool debugPermuation = false );

} // namespace PG::Gfx::PipelineManager
