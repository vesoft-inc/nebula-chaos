nebula_add_library(
    ssh_helper_obj OBJECT
    SshHelper.cpp
)

nebula_add_library(
    http_client_obj OBJECT
    HttpClient.cpp
)

nebula_add_subdirectory(test)
