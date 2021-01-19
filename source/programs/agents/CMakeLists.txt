# For ARM target only
if(${CROSS_TYPE} MATCHES "arm")
    #    include_directories(${PROJECT_SOURCE_DIR}/thirdparty/arm/include)
    #    add_library(ssl SHARED IMPORTED)
    #    set_property(TARGET ssl PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/thirdparty/arm/lib/libssl.so.1.0.0)
    #    add_library(crypto SHARED IMPORTED)
    #    set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/thirdparty/arm/lib/libcrypto.so.1.0.0)
endif()


file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(file ${files})
    get_filename_component(bfile ${file} NAME_WE)
        add_executable(${bfile} ${file})

                target_link_libraries(${bfile} ${COSMOS_LIBS} localzlib )
        install(TARGETS ${bfile} DESTINATION bin)

endforeach()

add_executable(agent_forward "${COSMOS_SOURCE}/core/programs/agents/agent_forward.cpp")
target_link_libraries(agent_forward CosmosAgent)
install(TARGETS agent_forward DESTINATION bin)

add_executable(agent_cpu "${COSMOS_SOURCE}/core/programs/agents/agent_cpu.cpp")
target_link_libraries(agent_cpu CosmosAgent CosmosDeviceDisk CosmosPrint)
install(TARGETS agent_cpu DESTINATION bin)

add_executable(agent_tunnel "${COSMOS_SOURCE}/core/programs/agents/agent_tunnel.cpp")
target_link_libraries(agent_tunnel CosmosAgent CosmosDeviceSerial)
install(TARGETS agent_tunnel DESTINATION bin)

add_executable(agent_exec "${COSMOS_SOURCE}/core/programs/agents/agent_exec.cpp")
target_link_libraries(agent_exec CosmosAgent)
install(TARGETS agent_exec DESTINATION bin)

add_executable(agent "${COSMOS_SOURCE}/core/programs/agents/agent.cpp")
target_link_libraries(agent CosmosAgent)
install(TARGETS agent DESTINATION bin)

add_executable(agent_file4 "${COSMOS_SOURCE}/core/programs/agents/agent_file4.cpp")
target_link_libraries(agent_file4 CosmosAgent CosmosTransfer CosmosSlip)
install(TARGETS agent_file4 DESTINATION bin)

target_link_libraries(agent_gather CosmosJson)
