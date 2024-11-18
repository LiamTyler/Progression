#pragma once

#include "core/scene.hpp"
#include "renderer/taskgraph/r_taskGraph.hpp"

namespace PG::Gfx
{

void Init_SceneData();
void Shutdown_SceneData();

void UpdateSceneData( Scene* scene );
void AddSceneRenderTasks( TaskGraphBuilder& builder, TGBTextureRef& litOutput, TGBTextureRef& sceneDepth );

} // namespace PG::Gfx
