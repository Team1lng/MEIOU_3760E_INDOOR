cmake_minimum_required(VERSION 3.10.2)
set(PROJECT "AmyApp.BIN")

set(PROJECT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPILER "/opt/arm-anykav500-linux-uclibcgnueabi/bin/arm-anykav500-linux-uclibcgnueabi-")
set(CMAKE_C_COMPILER ${COMPILER}gcc)
set(CMAKE_CXX_COMPILER ${COMPILER}g++)
set(CMAKE_OBJCOPY ${COMPILER}objcopy)
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -O2 -s -Wall -std=gnu11  -Werror")
set(CAMKE_ASM_FLAGS "${CAMKE_ASM_FLAGS}  -O2")
set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -O2 -s  -Wall -std=c++11  -Werror")

project(${PROJECT} LANGUAGES C CXX ASM)
#-fsanitize=address 
# set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -O0 -g -rdynamic -Wall -std=gnu11  -Werror -fno-omit-frame-pointer")

# set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}  -O0 -g")
# set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} -O0 -g -rdynamic -Wall -std=c++11  -Werror -fno-omit-frame-pointer")

# # 设置构建类型为Debug（显式指定）
# set(CMAKE_BUILD_TYPE Debug)

set(FIRMWARE_PATH ${PROJECT_PATH}/../AnyCloud37E_SDK_V1.10/tools/burntool/37E/)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
include_directories(
    ${PROJECT_PATH}/include/
    ${PROJECT_PATH}/common/
    ${PROJECT_PATH}/share/
    ${PROJECT_PATH}/driver/
    ${PROJECT_PATH}/share/lvgl/
    ${PROJECT_PATH}/share/libmad-0.15.1b/
    ${PROJECT_PATH}/share/onvif/
    ${PROJECT_PATH}/share/mxml-3.3.1/
    ${PROJECT_PATH}/share/call/include/
    ${PROJECT_PATH}/share/md5/
    ${PROJECT_PATH}/project/common/
    ${PROJECT_PATH}/share/speexdsp/
    ${PROJECT_PATH}/share/speexdsp/include/
    ${PROJECT_PATH}/share/jpeg-9f/
)

add_definitions(
    -DRES_BIN="/home/res/res.bin"#"./res.bin"
    -DLANGUAGE_XLS="/home/res/language.xls"
    -DOSIP_MT
    -DENABLE_SPEEX_AEC
)

# 添加子目录（每个子目录都会生成一个库）
add_subdirectory(common)
add_subdirectory(driver)
#add_subdirectory(project)
add_subdirectory(share)

# 创建合并的静态库
add_library(AmyApp STATIC
    $<TARGET_OBJECTS:common>    # 引用common的对象文件
    $<TARGET_OBJECTS:driver>    # 引用driver的对象文件
   # $<TARGET_OBJECTS:project>      # 引用project的对象文件
    $<TARGET_OBJECTS:mempool>    
    $<TARGET_OBJECTS:ringbuffer>    
    $<TARGET_OBJECTS:queues>   
    $<TARGET_OBJECTS:lvgl>   
#    $<TARGET_OBJECTS:md5>    
    $<TARGET_OBJECTS:freetype>  
    $<TARGET_OBJECTS:jpeg>     
    $<TARGET_OBJECTS:mad>     
    $<TARGET_OBJECTS:avi>      
    $<TARGET_OBJECTS:sqlite3>        
    $<TARGET_OBJECTS:live>              
    $<TARGET_OBJECTS:tuya>
    $<TARGET_OBJECTS:onvif>
    $<TARGET_OBJECTS:mxml> 
    $<TARGET_OBJECTS:osip2> 
    $<TARGET_OBJECTS:sha1>  
    $<TARGET_OBJECTS:call> 
    $<TARGET_OBJECTS:xls>
    $<TARGET_OBJECTS:iconv>
	    $<TARGET_OBJECTS:speexdsp>
)

# 匹配 page 目录下所有 .c 文件
file(GLOB PAGE_SRC "${PROJECT_PATH}/project/page/*.c")
# 匹配 business/ 下所有 .c 文件，包括一级、二级子目录（递归）
file(GLOB_RECURSE BUSINESS_SRC "${PROJECT_PATH}/project/business/**/*.c")
file(GLOB PAGE_COMMON_SRC "${PROJECT_PATH}/project/common/*.c")
file(GLOB PAGE_TEST_SRC "${PROJECT_PATH}/project/page/test/*.c")
add_executable(${PROJECT} 
    ${PROJECT_PATH}/project/app_main.c
    ${PROJECT_PATH}/project/gpio.c
    ${PROJECT_PATH}/project/pwm.c
    ${PROJECT_PATH}/project/monitor.c
    ${PROJECT_PATH}/project/intercom.c
    ${PROJECT_PATH}/project/key.c
    ${PAGE_SRC}  # 包含 page 目录下所有 .c 文件
    ${BUSINESS_SRC}  # 包含 call 目录下所有 .c 文件
    ${PAGE_COMMON_SRC}  # 包含 page/common 目录下所有 .c 文件
    ${PAGE_TEST_SRC} # 包含 page/test 目录下所有 .c 文件
)

set(VAR ${VAR} base64)

set(PLATFORM_PATH ${PROJECT_PATH}/lib)
target_link_libraries(${PROJECT} -Wl,--start-group
    m
    pthread
    dl
	# share lib
	${VAR}    ${PLATFORM_PATH}/libplat_ai.a
    ${PLATFORM_PATH}/libplat_vqe.a
    ${PLATFORM_PATH}/libplat_mem.a
    ${PLATFORM_PATH}/libplat_thread.a
    ${PLATFORM_PATH}/libakaudiofilter.a
    ${PLATFORM_PATH}/libplat_tde.a
    ${PLATFORM_PATH}/libplat_common.a
    ${PLATFORM_PATH}/libplat_dbg.a
    ${PLATFORM_PATH}/libplat_osal.a
    ${PLATFORM_PATH}/libmpi_venc.a
    ${PLATFORM_PATH}/libakv_encode.a
    ${PLATFORM_PATH}/libakv_decode.a
    ${PLATFORM_PATH}/libmpi_vdec.a
    ${PLATFORM_PATH}/libplat_ao.a
    ${PLATFORM_PATH}/libtuyaos.a
   # ${PLATFORM_PATH}/libplat_log.a
    
    AmyApp
    # ========== 修改3: 链接时添加生成core dump的选项 ==========
    # -Wl,--allow-multiple-definition
    -rdynamic  # 导出所有符号，确保core dump能解析函数名
    -Wl,--end-group)

set(ROOTFS_PATH ${PROJECT_PATH}/../AnyCloud37E_SDK_V1.10/rootfs/parttion)
set(IMG_PATH ${PROJECT_PATH}/../AnyCloud37E_SDK_V1.10/tools/burntool/37E)
## 生成bin文件
add_custom_command(
    TARGET ${PROJECT}
    POST_BUILD
  #  COMMAND cp ${PROJECT} ${PROJECT_PATH}/../AnyCloud37E_SDK_V1.10/rootfs/parttion/app/
    COMMAND ${ROOTFS_PATH}/mksquashfs ${ROOTFS_PATH}/app  ${IMG_PATH}/app.img  -noappend -comp xz
    COMMENT "Generating ${PROJECT}.bin "
)


add_custom_command(
    TARGET ${PROJECT}
    POST_BUILD
    # COMMAND rm ${ROOTFS_PATH}/res/res.bin
    COMMAND cp res.bin  ${ROOTFS_PATH}/res
    COMMAND cp ../project/resource/language.xls  ${ROOTFS_PATH}/res
    COMMAND cp -r ../project/resource/wallpaper  ${ROOTFS_PATH}/res
    COMMAND cp -r AmyApp.BIN ${ROOTFS_PATH}/app
    COMMAND gcc ${PROJECT_PATH}/script/generate_firmware.c -o generate_firmware
    COMMAND  ./generate_firmware
    COMMAND rm generate_firmware
    COMMENT "Generating firmware finish"
)