add_definitions(
)
include_directories(
    .
)
add_library(mxml OBJECT
        mxml-file.c
        mxml-private.c
        mxml-node.c
        mxml-attr.c
        mxml-entity.c
        mxml-string.c
        mxml-search.c
        mxml-get.c
 )
