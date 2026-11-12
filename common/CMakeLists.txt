
# 替换为当前目录下所有 .c 文件
file(GLOB src_c "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB src_cpp "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(common OBJECT
  ${src_c}
  ${src_cpp}
)
