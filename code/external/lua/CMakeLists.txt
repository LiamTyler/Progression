cmake_minimum_required(VERSION 3.5)
project(lua)

if(UNIX AND NOT APPLE)
    add_definitions(-DLUA_USE_POSIX)
endif()
file(GLOB_RECURSE LUA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
list(REMOVE_ITEM LUA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lua.c)
list(REMOVE_ITEM LUA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/luac.c)
add_library(lua ${LUA_SRC})