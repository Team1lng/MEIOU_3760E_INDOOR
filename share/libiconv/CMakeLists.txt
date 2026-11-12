add_definitions(
    -DHAVE_CONFIG_H
)

include_directories(
    .
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/libcharset/
  #  ${CMAKE_CURRENT_SOURCE_DIR}/srclib/
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/libcharset/include/
)

add_library(iconv OBJECT

    lib/iconv.c
    libcharset/lib/localcharset.c
  #  srclib/free.c
  #  srclib/malloc.c
)
