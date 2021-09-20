function(COPY_FILE_IF_DIFFERENT src dst)
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src} ${dst})
endfunction()

function(CONFIG_TIME_COMPILE source_dir build_dir CONFIG)
    file(MAKE_DIRECTORY ${build_dir})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=${CONFIG} ${source_dir}
        WORKING_DIRECTORY ${build_dir}
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build . --config ${CONFIG}
        WORKING_DIRECTORY ${build_dir}
    )
endfunction()


function(CONFIG_TIME_COMPILE_DEBUG_AND_RELEASE source_dir build_dir)
    if(WIN32)
        CONFIG_TIME_COMPILE(${source_dir} ${build_dir} Debug)
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build . --config Release
            WORKING_DIRECTORY ${build_dir}
        )
    else()
        CONFIG_TIME_COMPILE(${source_dir} ${build_dir}/Debug Debug)
        CONFIG_TIME_COMPILE(${source_dir} ${build_dir}/Release Release)
    endif()
endfunction()

# Copy each file from source_dir that globs with prefix* into
# dst_bin_dir if the file extension is contained in the BINARY_EXTENSIONS list
# dst_lib_dir if the file extension is contained in the LIB_EXTENSIONS list
# otherwise it is not copied
function(COPY_BUILD_FILES source_dir dst_bin_dir dst_lib_dir prefix)
    file(GLOB files "${source_dir}/${prefix}*")
    set(BINARY_EXTENSIONS ".exe" ".dll" ".so" "")
    set(LIB_EXTENSIONS ".lib" ".a" ".pdb")
    foreach(f ${files})
        get_filename_component(ext ${f} EXT)
        get_filename_component(name ${f} NAME)
        if("${ext}" IN_LIST BINARY_EXTENSIONS)
            COPY_FILE_IF_DIFFERENT(${f} "${dst_bin_dir}/${name}")
        elseif("${ext}" IN_LIST LIB_EXTENSIONS)
			COPY_FILE_IF_DIFFERENT(${f} "${dst_lib_dir}/${name}")
        endif()
    endforeach()
endfunction()


function(SET_TARGET_COMPILE_OPTIONS_DEFAULT)
    foreach(arg IN LISTS ARGN)
        if(UNIX AND NOT APPLE)
            target_compile_options(${arg} PRIVATE -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch -Wno-format)
            target_compile_options(${arg} PRIVATE $<$<CONFIG:DEBUG>: -g>)
            target_compile_options(${arg} PRIVATE $<$<CONFIG:RELEASE>: -O2>)
            target_compile_options(${arg} PRIVATE $<$<CONFIG:SHIP>:-O3>)
        elseif(MSVC)
            target_compile_options(${arg} PRIVATE $<$<CONFIG:DEBUG>: /Od>)
            target_compile_options(${arg} PRIVATE $<$<CONFIG:RELEASE>: /O2>)
            target_compile_options(${arg} PRIVATE $<$<CONFIG:SHIP>: /O2>)
            target_compile_options(${arg} PRIVATE /MP)
            target_compile_definitions(${arg} PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        target_compile_definitions(${arg} PRIVATE $<$<CONFIG:Debug>:CMAKE_DEFINE_DEBUG_BUILD> $<$<CONFIG:Release>:CMAKE_DEFINE_RELEASE_BUILD>)
    endforeach()
endfunction()


function(SET_TARGET_POSTFIX)
    foreach(arg IN LISTS ARGN)
        set_target_properties(
            ${arg}
            PROPERTIES
            DEBUG_POSTFIX _debug
            RELEASE_POSTFIX _release
            SHIP_POSTFIX _ship
        )
    endforeach()
endfunction()

function(SET_TARGET_AS_DEFAULT_VS_PROJECT proj)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${proj})
endfunction()