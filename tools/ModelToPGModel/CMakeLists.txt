project(modelToPGModel)

include(set_postfix)

include_directories(
    ${PROGRESSION_DIR}
    ${PROGRESSION_DIR}/ext/
    ${PROGRESSION_DIR}/src
    ${PROGRESSION_DIR}/ext/assimp/include
)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${PROGRESSION_DIR}/src/core/assert.hpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.cpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.hpp
    ${PROGRESSION_DIR}/src/utils/filesystem.cpp
    ${PROGRESSION_DIR}/src/utils/filesystem.hpp
    ${PROGRESSION_DIR}/src/utils/logger.cpp
    ${PROGRESSION_DIR}/src/utils/logger.hpp
    ${PROGRESSION_DIR}/src/utils/serializer.cpp
    ${PROGRESSION_DIR}/src/utils/serializer.hpp
    
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.h
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.cpp
)

set(LIBS
    assimp
)

add_executable(${PROJECT_NAME} ${SRC})
SET_TARGET_POSTFIX(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()