include_directories(
  ./
  ${PROJECT_PATH}/common/
  ${PROJECT_PATH}/driver/

)

set(src
    audio_input.c
    audio_output.c
    video_decode.c
    mjpeg_encode.c
  #  video_input.c
    video_output.c
    gpio_ctrl.c
    i2c_ctrl.c
    pwm_ctrl.c
  #  sgm6502.c
  #  tp9950.c
)

add_library(driver OBJECT
  ${src}
)