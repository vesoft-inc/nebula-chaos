
find_program(CURL_EXECUTABLE
             NAMES curl
             PATHS ${CURL_EXECUTABLE_DIR}
             DOC "path to the curl executable")

message(STATUS "curl path: ${CURL_EXECUTABLE}")
