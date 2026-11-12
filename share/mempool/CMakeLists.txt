
include_directories(
  ${PROJECT_PATH}/
)

set(src
    mempool.c
)

add_library(mempool OBJECT
  ${src}
)