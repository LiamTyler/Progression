function( SET_TARGET_POSTFIX )
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