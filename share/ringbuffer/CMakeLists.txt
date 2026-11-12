
include_directories(
  ${PROJECT_PATH}/
  ${PROJECT_PATH}/../rt-thread-4.1.x/components/libc/posix/pthreads/
  ${PROJECT_PATH}/../rt-thread-4.1.x/include/
  ${PROJECT_PATH}/../rt-thread-4.1.x/bsp/xm620/
  ${PROJECT_PATH}/../rt-thread-4.1.x/components/finsh/
)

set(src
    ringbuffer.c
    test_ringbuffer.c
)

add_library(ringbuffer OBJECT
  ${src}
)