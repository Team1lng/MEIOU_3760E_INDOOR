#include "tuya_sdk.h"
#include "tuya_ipc_event.h"
#include "tuya_ipc_sdk_init.h"
#include "tuya_ipc_upgrade.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_ipc_media_adapter.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_sdk_init.h"
#include "Amy_log.h"
#include "Amy_sdk.h"


int tuya_stream_storage_init(void) {
    //本地存储功能在本机已实现，不需要在这处理。
    return 0;
}