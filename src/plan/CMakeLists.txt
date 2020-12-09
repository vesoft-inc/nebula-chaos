set(NEBULA_CHAOS_OBJS
    $<TARGET_OBJECTS:client_nebula_graph_client_obj>
)

nebula_add_library(
    nebula_instance_obj OBJECT
    NebulaInstance.cpp
    ${NEBULA_CHAOS_OBJS}
)

nebula_add_library(
    nebula_plan_obj OBJECT
    NebulaChaosPlan.cpp
    NebulaAction.cpp
    ${NEBULA_CHAOS_OBJS}
)

nebula_add_subdirectory(client)
nebula_add_subdirectory(test)
