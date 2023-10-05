project(RemoteConsole)

include(helpful_functions)
include(source_files)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/remote_console_main.cpp
    
    ${CODE_DIR}/core/time.cpp
    ${CODE_DIR}/core/time.hpp
	
    ${CODE_DIR}/shared/logger.cpp
    ${CODE_DIR}/shared/logger.hpp
    ${CODE_DIR}/shared/math.cpp
    ${CODE_DIR}/shared/math.hpp
    ${CODE_DIR}/shared/platform_defines.hpp
)

set(ALL_FILES ${SRC} ${ISOCLINE_EXT_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})
set_source_files_properties(${ISOCLINE_EXT_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
add_executable(${PROJECT_NAME} ${ALL_FILES})

set(LIBS OpenMP::OpenMP_CXX isocline)
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)
target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
