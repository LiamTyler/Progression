cmake_minimum_required(VERSION 3.0)
project(ModelExporter)

include(helpful_functions)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_main.cpp
    ${PROGRESSION_DIR}/code/core/pixel_formats.cpp
    ${PROGRESSION_DIR}/code/core/pixel_formats.hpp
	${PROGRESSION_DIR}/code/core/time.cpp
    ${PROGRESSION_DIR}/code/core/time.hpp
    ${PROGRESSION_DIR}/code/shared/assert.hpp
    ${PROGRESSION_DIR}/code/shared/filesystem.cpp
    ${PROGRESSION_DIR}/code/shared/filesystem.hpp
    ${PROGRESSION_DIR}/code/shared/logger.cpp
    ${PROGRESSION_DIR}/code/shared/logger.hpp
)

set(LIBS assimp-vc142-mt OpenMP::OpenMP_CXX)

add_executable(${PROJECT_NAME} ${SRC})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROGRESSION_DIR}/code/
    ${PROGRESSION_DIR}/code/external/
    ${PROGRESSION_DIR}/code/external/assimp/include
	${CMAKE_BINARY_DIR}/ext/assimp/include
)