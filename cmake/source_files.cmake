set(CODE_DIR ${PROGRESSION_DIR}/code)

set(
    ASSET_SRC
    
    ${CODE_DIR}/asset/types/base_asset.hpp
    ${CODE_DIR}/asset/types/gfx_image.cpp
    ${CODE_DIR}/asset/types/gfx_image.hpp
    ${CODE_DIR}/asset/types/material.cpp
    ${CODE_DIR}/asset/types/material.hpp
    ${CODE_DIR}/asset/types/model.cpp
    ${CODE_DIR}/asset/types/model.hpp
    ${CODE_DIR}/asset/types/script.cpp
    ${CODE_DIR}/asset/types/script.hpp
    ${CODE_DIR}/asset/types/shader.cpp
    ${CODE_DIR}/asset/types/shader.hpp
    ${CODE_DIR}/asset/types/textureset.cpp
    ${CODE_DIR}/asset/types/textureset.hpp
    ${CODE_DIR}/asset/shader_preprocessor.cpp
    ${CODE_DIR}/asset/shader_preprocessor.hpp
    
    ${CODE_DIR}/asset/asset_manager.cpp
    ${CODE_DIR}/asset/asset_manager.hpp
    ${CODE_DIR}/asset/asset_versions.hpp
)

set(
    ASSET_PARSE_SRC
    
    ${CODE_DIR}/asset/parsing/base_asset_parse.cpp
    ${CODE_DIR}/asset/parsing/base_asset_parse.hpp
    ${CODE_DIR}/asset/parsing/gfx_image_parse.cpp
    ${CODE_DIR}/asset/parsing/gfx_image_parse.hpp
    ${CODE_DIR}/asset/parsing/material_parse.cpp
    ${CODE_DIR}/asset/parsing/material_parse.hpp
    ${CODE_DIR}/asset/parsing/model_parse.cpp
    ${CODE_DIR}/asset/parsing/model_parse.hpp
    ${CODE_DIR}/asset/parsing/script_parse.cpp
    ${CODE_DIR}/asset/parsing/script_parse.hpp
    ${CODE_DIR}/asset/parsing/shader_parse.cpp
    ${CODE_DIR}/asset/parsing/shader_parse.hpp
    ${CODE_DIR}/asset/parsing/textureset_parse.cpp
    ${CODE_DIR}/asset/parsing/textureset_parse.hpp
)

set(
    ECS_SRC
    
    ${CODE_DIR}/ecs/component_factory.cpp
    ${CODE_DIR}/ecs/component_factory.hpp
    ${CODE_DIR}/ecs/components/entity_metadata.hpp
    ${CODE_DIR}/ecs/components/model_renderer.hpp
    ${CODE_DIR}/ecs/components/transform.cpp
    ${CODE_DIR}/ecs/components/transform.hpp
    ${CODE_DIR}/ecs/ecs.cpp
    ${CODE_DIR}/ecs/ecs.hpp
)

set(
    PRIMARY_SHARED_SRC
    
    ${CODE_DIR}/shared/assert.hpp
    ${CODE_DIR}/shared/bitops.hpp
    ${CODE_DIR}/shared/core_defines.hpp
    ${CODE_DIR}/shared/file_dependency.cpp
    ${CODE_DIR}/shared/file_dependency.hpp
    ${CODE_DIR}/shared/filesystem.cpp
    ${CODE_DIR}/shared/filesystem.hpp
    ${CODE_DIR}/shared/logger.cpp
    ${CODE_DIR}/shared/logger.hpp
    ${CODE_DIR}/shared/math.cpp
    ${CODE_DIR}/shared/math.hpp
    ${CODE_DIR}/shared/platform_defines.hpp
    ${CODE_DIR}/shared/random.cpp
    ${CODE_DIR}/shared/random.hpp
    ${CODE_DIR}/shared/string.cpp
    ${CODE_DIR}/shared/string.hpp
)