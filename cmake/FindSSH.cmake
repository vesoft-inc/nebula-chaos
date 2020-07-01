
find_program(SSH_EXECUTABLE
             NAMES ssh
             PATHS ${SSH_EXECUTABLE_DIR}
             DOC "path to the ssh executable")

message(STATUS "ssh path: ${SSH_EXECUTABLE}")


find_program(MAIL_EXECUTABLE
             NAMES mail
             PATHS ${MAIL_EXECUTABLE_DIR}
             DOC "path to the mail executable")

message(STATUS "mail path: ${MAIL_EXECUTABLE}")


find_program(ECHO_EXECUTABLE
             NAMES echo
             PATHS ${ECHO_EXECUTABLE_DIR}
             DOC "path to the echo executable")

message(STATUS "echo path: ${ECHO_EXECUTABLE}")

