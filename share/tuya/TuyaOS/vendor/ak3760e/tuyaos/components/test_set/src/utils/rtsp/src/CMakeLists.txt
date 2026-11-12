include_directories(
	${TOPDIR}/bases/base/include/
	${TOPDIR}/bases/net/include/
	${TOPDIR}/bases/simulation/include/
	${TOPDIR}/uwm/hwl/include/
	${TOPDIR}/uwm/rtsp/include/
	${TOPDIR}/skyeye/include/
	${TOPDIR}/prebuild/tuya/include/
	${TOPDIR}/prebuild/3rdparty/include/pcap/
	${TOPDIR}/prebuild/3rdparty/include/wireless/
)

file(GLOB_RECURSE HEADERFILES "../include/*")
aux_source_directory(. DIR_SRCS)

add_library(ty_rtsp ${DIR_SRCS} ${HEADERFILES})
