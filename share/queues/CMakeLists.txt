
include_directories(
  ${PROJECT_PATH}/
  ${PROJECT_PATH}/share/mempool/
)

set(src
    queues.c
    test_queues.c
)

add_library(queues OBJECT
  ${src}
)