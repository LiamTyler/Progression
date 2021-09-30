cmake_minimum_required(VERSION 3.0)
project(LZ4)

set(
    SRCS
    lz4.c 
    lz4hc.c 
    lz4frame.c 
    xxhash.c 
)

set(
    HEADERS
    lz4.h 
    lz4hc.h 
    lz4frame.h 
    xxhash.h 
)

add_library(lz4 STATIC ${SRCS} ${HEADERS})

if(MSVC)
    target_compile_options(lz4 PRIVATE "/MP")
endif()