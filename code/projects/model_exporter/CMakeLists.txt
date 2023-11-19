project(ModelExporter)

include(helpful_functions)
include(source_files)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_common.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_materials.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_materials.hpp
    ${PROGRESSION_DIR}/code/core/pixel_formats.cpp
    ${PROGRESSION_DIR}/code/core/pixel_formats.hpp
	${PROGRESSION_DIR}/code/core/time.cpp
    ${PROGRESSION_DIR}/code/core/time.hpp
    
    ${CODE_DIR}/asset/asset_file_database.cpp
    ${CODE_DIR}/asset/asset_file_database.hpp
    ${CODE_DIR}/asset/pmodel.cpp
    ${CODE_DIR}/asset/pmodel.hpp
    ${CODE_DIR}/asset/parsing/asset_parsers.cpp
    ${CODE_DIR}/asset/parsing/asset_parsers.hpp
    
    ${PRIMARY_SHARED_SRC}
    ${CODE_DIR}/shared/json_parsing.cpp
    ${CODE_DIR}/shared/json_parsing.hpp
)

set(
	EXTERNALS
    ${CODE_DIR}/external/getopt/getopt.c
    ${CODE_DIR}/external/getopt/getopt.h
)

set(ALL_FILES ${SRC} ${EXTERNALS})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})
add_executable(${PROJECT_NAME} ${ALL_FILES})

set(LIBS ${ASSIMP_LIBS} OpenMP::OpenMP_CXX)

SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)
target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDE_DIRS} ${ASSIMP_INCLUDES})
