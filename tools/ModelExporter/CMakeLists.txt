project(ModelExporter)

include(set_postfix)
include(set_compile_options_default)

include_directories(
    ${PROGRESSION_DIR}
    ${PROGRESSION_DIR}/ext/
    ${PROGRESSION_DIR}/src
    ${PROGRESSION_DIR}/ext/assimp/include
)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/model_exporter_main.cpp
    ${PROGRESSION_DIR}/src/core/assert.hpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.cpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.hpp
	${PROGRESSION_DIR}/src/core/time.cpp
    ${PROGRESSION_DIR}/src/core/time.hpp
    ${PROGRESSION_DIR}/src/utils/filesystem.cpp
    ${PROGRESSION_DIR}/src/utils/filesystem.hpp
    ${PROGRESSION_DIR}/src/utils/logger.cpp
    ${PROGRESSION_DIR}/src/utils/logger.hpp
)

set(LIBS
    assimp
	OpenMP::OpenMP_CXX
)

add_executable(${PROJECT_NAME} ${SRC})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})
