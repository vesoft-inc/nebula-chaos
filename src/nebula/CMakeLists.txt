nebula_add_library(
    nebula_instance_obj OBJECT
    NebulaInstance.cpp
)

nebula_add_library(
    nebula_plan_obj OBJECT
    NebulaChaosPlan.cpp
    NebulaAction.cpp
)

nebula_add_subdirectory(interface)
nebula_add_subdirectory(client)
nebula_add_subdirectory(test)
