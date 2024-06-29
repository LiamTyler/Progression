project(BRDFIntegrate)

include(helpful_functions)
include(source_files)

set(CODE_DIR ${PROGRESSION_DIR}/code)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/brdf_integrate_main.cpp
    
    ${CODE_DIR}/core/low_discrepancy_sampling.cpp
    ${CODE_DIR}/core/low_discrepancy_sampling.hpp
    ${CODE_DIR}/renderer/brdf_functions.cpp
    ${CODE_DIR}/renderer/brdf_functions.hpp
    
    ${PRIMARY_SHARED_SRC}
)

set(
	EXTERNALS
)

set(ALL_FILES ${SRC} ${EXTERNALS})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})


add_executable(${PROJECT_NAME} ${ALL_FILES})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})

set(LIBS OpenMP::OpenMP_CXX)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS}
    debug ${IMAGELIB_LIBS_DEBUG} 
    optimized ${IMAGELIB_LIBS}
)
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${IMAGELIB_INCLUDES}
)
