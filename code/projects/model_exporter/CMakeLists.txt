cmake_minimum_required(VERSION 3.0)
project(ModelExporter)

include(helpful_functions)
include(source_files)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_main.cpp
    ${PROGRESSION_DIR}/code/core/pixel_formats.cpp
    ${PROGRESSION_DIR}/code/core/pixel_formats.hpp
	${PROGRESSION_DIR}/code/core/time.cpp
    ${PROGRESSION_DIR}/code/core/time.hpp
    
    ${PRIMARY_SHARED_SRC}
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SRC})

set(LIBS ${ASSIMP_LIBS} OpenMP::OpenMP_CXX)

add_executable(${PROJECT_NAME} ${SRC})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)
target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDE_DIRS} ${ASSIMP_INCLUDES})