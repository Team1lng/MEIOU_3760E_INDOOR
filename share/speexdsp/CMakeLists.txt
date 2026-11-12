add_library(speexdsp STATIC
    mdf.c
    fftwrap.c
    kiss_fft.c
    kiss_fftr.c
)

target_include_directories(speexdsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(speexdsp PRIVATE
    FIXED_POINT
    EXPORT=
    USE_KISS_FFT
    DISABLE_WARNINGS
)
