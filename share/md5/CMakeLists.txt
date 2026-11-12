add_definitions(
        -Wno-stringop-truncation
        -Wno-format
        -Wno-restrict
)

add_library(md5 OBJECT
        md5.c
 )