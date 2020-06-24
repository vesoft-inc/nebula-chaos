nebula_add_library(
    nebula_instance_obj OBJECT
    NebulaInstance.cpp
)

nebula_add_library(
    nebula_actions_obj OBJECT
    NebulaAction.cpp
)

nebula_add_subdirectory(test)
