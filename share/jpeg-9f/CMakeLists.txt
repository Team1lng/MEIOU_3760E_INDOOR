
include_directories(
  ${PROJECT_PATH}/
)

set(src
  jpeg_decode.c
  jerror.c
  jdapimin.c
  jcomapi.c
  jdinput.c
  jutils.c
  jdmarker.c
  jmemmgr.c
  jmemansi.c
  jdapistd.c
  jdmaster.c
  jdmainct.c
  jdmerge.c
  jdpostct.c
  jdsample.c
  jdcolor.c
  jdcoefct.c
  jdhuff.c
  jdarith.c
  jdatasrc.c
  jddctmgr.c
  jidctint.c
  jquant1.c
  jquant2.c
  jidctfst.c
  jidctflt.c
  jaricom.c
)

add_library(jpeg OBJECT
  ${src}
)