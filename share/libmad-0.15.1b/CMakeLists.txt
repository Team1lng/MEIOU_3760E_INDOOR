add_definitions(
  -DHAVE_CONFIG_H
  -DFPM_ARM
  -DASO_INTERLEAVE1 
  #-DASO_IMDCT
)

set(src
    version.c
    fixed.c
    bit.c
    timer.c
    stream.c
    frame.c
    synth.c
    decoder.c
    layer12.c
    layer3.c
    huffman.c
  #  imdct_l_arm.S
)

add_library(mad OBJECT
  ${src}
)