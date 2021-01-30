#pragma once

namespace PG
{

class Scene;

bool InitPipelineGenerator();

void ShutdownPipelineGenerator();

bool GenerateMissingPipelines( Scene* scene );

} // namespace PG