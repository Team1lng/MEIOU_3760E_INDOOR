add_definitions(
        -Wno-char-subscripts
)
include_directories(
    .
)
add_library(sha1 OBJECT
        ./sha1.c
     #   ./lib/ubc_check.c
 )
