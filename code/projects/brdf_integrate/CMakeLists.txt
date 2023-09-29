project(BRDFIntegrate)

include(helpful_functions)
include(source_files)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/brdf_integrate_main.cpp
    
    ${PRIMARY_SHARED_SRC}
)

set(
	EXTERNALS
)

set(ALL_FILES ${SRC} ${EXTERNALS} ${IMAGELIB_EXT_FILES})
set_source_files_properties(${IMAGELIB_EXT_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})


add_executable(${PROJECT_NAME} ${ALL_FILES})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})

set(LIBS OpenMP::OpenMP_CXX)# ${VULKAN_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS}
    debug ${IMAGELIB_LIBS_DEBUG} 
    optimized ${IMAGELIB_LIBS}
)
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    #${Vulkan_INCLUDE_DIR}
    ${IMAGELIB_INCLUDES}
)
