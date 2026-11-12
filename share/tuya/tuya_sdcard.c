#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <dirent.h>

#include "tkl_storage.h"
#include "tal_system.h"
#include "utilities/uni_log.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_media.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_media_stream_event.h"
#include "ty_sdk_common.h"


STREAM_STORAGE_WRITE_MODE_E tuya_ipc_sd_get_mode_config(void) {
    return SS_WRITE_MODE_NONE;
}