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

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SRC})

set(LIBS OpenMP::OpenMP_CXX)

add_executable(${PROJECT_NAME} ${SRC})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)
target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
