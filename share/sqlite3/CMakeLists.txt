set(src
    sqlite3_common.c
    sqlite3.c
)

add_library(sqlite3 OBJECT
  ${src}
)

# amalgamation 在 balance_nonroot 等处会触发 GCC 误报 array-bounds，与全局 -Werror 冲突
target_compile_options(sqlite3 PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-array-bounds>)

target_compile_definitions(sqlite3 PRIVATE 
  SQLITE_OMIT_LOAD_EXTENSION
  -DHAVE_NANOSLEEP=0 
  -DHAVE_USLEEP
  -DSQLITE_OS_UNIX
  -DSQLITE_OS_KV_OPTIONAL
  )