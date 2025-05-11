project(Sandbox)

include(helpful_functions)
include(source_files)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/sandbox_main.cpp
    
    ${CODE_DIR}/core/time.cpp
    ${CODE_DIR}/core/time.hpp
    
    ${CODE_DIR}/shared/lz4_compressor.cpp
    ${CODE_DIR}/shared/lz4_compressor.hpp
	
    ${PRIMARY_SHARED_SRC}
)

set(
	EXTERNALS
    ${CODE_DIR}/external/memory_map/MemoryMapped.h
    ${CODE_DIR}/external/memory_map/MemoryMapped.cpp
)

set(ALL_FILES ${SRC} ${EXTERNALS} ${IMAGELIB_EXT_FILES} ${MSDFGEN_EXT_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})
set_source_files_properties(${IMAGELIB_EXT_FILES} ${MSDFGEN_EXT_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/
    ${COMMON_INCLUDE_DIRS} ${IMAGELIB_INCLUDES}
)

SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${MSDFGEN_COMPILE_DEFS})
target_link_libraries(${PROJECT_NAME} PUBLIC debug OpenMP::OpenMP_CXX
	${IMAGELIB_LIBS_DEBUG} lz4
)
target_link_libraries(${PROJECT_NAME} PUBLIC optimized OpenMP::OpenMP_CXX
    ${IMAGELIB_LIBS} lz4
)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)
