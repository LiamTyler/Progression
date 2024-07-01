#pragma once

#include "asset/types/pipeline.hpp"

namespace PG::Gfx::PipelineManager
{

void Init();
void Shutdown();
VkPipelineCache GetPipelineCache();

void CreatePipeline( Pipeline* pipeline, const PipelineCreateInfo& createInfo );
Pipeline* GetPipeline( const std::string& name, bool debugPermuation = false );

} // namespace PG::Gfx::PipelineManager
