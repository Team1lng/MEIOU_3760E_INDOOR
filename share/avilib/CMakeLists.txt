

set(src
    avilib.c
    video_play.c
    video_record.c
)

add_library(avi OBJECT
  ${src}
)