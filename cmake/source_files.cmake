set(CODE_DIR ${PROGRESSION_DIR}/code)

set(
    ASSET_SRC
    
    ${CODE_DIR}/asset/types/base_asset.cpp
    ${CODE_DIR}/asset/types/base_asset.hpp
    ${CODE_DIR}/asset/types/font.cpp
    ${CODE_DIR}/asset/types/font.hpp
    ${CODE_DIR}/asset/types/gfx_image.cpp
    ${CODE_DIR}/asset/types/gfx_image.hpp
    ${CODE_DIR}/asset/types/material.cpp
    ${CODE_DIR}/asset/types/material.hpp
    ${CODE_DIR}/asset/types/model.cpp
    ${CODE_DIR}/asset/types/model.hpp
    ${CODE_DIR}/asset/types/pipeline.cpp
    ${CODE_DIR}/asset/types/pipeline.hpp
    ${CODE_DIR}/asset/types/script.cpp
    ${CODE_DIR}/asset/types/script.hpp
    ${CODE_DIR}/asset/types/shader.cpp
    ${CODE_DIR}/asset/types/shader.hpp
    ${CODE_DIR}/asset/types/textureset.cpp
    ${CODE_DIR}/asset/types/textureset.hpp
	${CODE_DIR}/asset/types/ui_layout.cpp
    ${CODE_DIR}/asset/types/ui_layout.hpp
    ${CODE_DIR}/asset/shader_preprocessor.cpp
    ${CODE_DIR}/asset/shader_preprocessor.hpp
    
    ${CODE_DIR}/asset/asset_manager.cpp
    ${CODE_DIR}/asset/asset_manager.hpp
    ${CODE_DIR}/asset/asset_versions.hpp
    ${CODE_DIR}/asset/pmodel.cpp
    ${CODE_DIR}/asset/pmodel.hpp
)

set(
    ASSET_PARSE_SRC
    
    ${CODE_DIR}/asset/parsing/asset_parsers.cpp
    ${CODE_DIR}/asset/parsing/asset_parsers.hpp
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
    UI_SRC
    
    ${CODE_DIR}/ui/ui_data_structures.cpp
    ${CODE_DIR}/ui/ui_data_structures.hpp
	${CODE_DIR}/ui/ui_element.cpp
    ${CODE_DIR}/ui/ui_element.hpp
    ${CODE_DIR}/ui/ui_system.cpp
    ${CODE_DIR}/ui/ui_system.hpp
    ${CODE_DIR}/ui/ui_text.cpp
    ${CODE_DIR}/ui/ui_text.hpp
)

set(
    DATA_STRUCTURES
    
    ${CODE_DIR}/data_structures/circular_array.hpp
    ${CODE_DIR}/data_structures/free_slot_bit_array.hpp
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
    ${CODE_DIR}/shared/math.hpp
    ${CODE_DIR}/shared/math_base.hpp
    ${CODE_DIR}/shared/math_matrix.cpp
    ${CODE_DIR}/shared/math_matrix.hpp
    ${CODE_DIR}/shared/math_vec.cpp
    ${CODE_DIR}/shared/math_vec.hpp
    ${CODE_DIR}/shared/platform_defines.hpp
    ${CODE_DIR}/shared/random.cpp
    ${CODE_DIR}/shared/random.hpp
    ${CODE_DIR}/shared/string.cpp
    ${CODE_DIR}/shared/string.hpp
    
    ${DATA_STRUCTURES}
)

set(SHADER_DIR ${PROGRESSION_DIR}/assets/shaders)
file(GLOB SHADER_ROOT_FILES ${SHADER_DIR}/*.*)
file(GLOB SHADER_DEBUG_FILES ${SHADER_DIR}/debug/*)
file(GLOB SHADER_LIB_FILES ${SHADER_DIR}/lib/*)
file(GLOB SHADER_UI_FILES ${SHADER_DIR}/ui/*)
set(SHADERS ${SHADER_ROOT_FILES} ${SHADER_DEBUG_FILES} ${SHADER_LIB_FILES} ${SHADER_UI_FILES})

file(GLOB_RECURSE C_SHARED_SHADERS ${SHADER_DIR}/c_shared/*)
