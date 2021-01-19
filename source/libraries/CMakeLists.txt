file(GLOB directs "./*")

foreach(direct ${directs})
    get_filename_component(bext ${direct} EXT)

    if (${bext} MATCHES "")



    elseif (${direct} MATCHES "flthard_base")
    elseif (${direct} MATCHES "flthard_iptun")
    elseif (${direct} MATCHES "flthard_isc")

    else()
        get_filename_component(bdirect ${direct} NAME_WE)
        add_subdirectory(${PROJECT_SOURCE_DIR}/libraries/${bdirect})
    endif()

endforeach()


