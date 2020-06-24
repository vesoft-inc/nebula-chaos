
find_program(SSH_EXECUTABLE
             NAMES ssh
             PATHS ${SSH_EXECUTABLE_DIR}
             DOC "path to the ssh executable")

message(STATUS "ssh path: ${SSH_EXECUTABLE}")
