#include "tuya_sdk.h"
#include "tuya_ipc_event.h"
#include "tuya_ipc_sdk_init.h"
#include "tuya_ipc_upgrade.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_ipc_media_adapter.h"
#include "tuya_ipc_media_stream.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_sdk_init.h"
#include "tuya_tmm_control.h"
#include "tal_workqueue.h"
#include "tal_workq_service.h"
#include "Amy_log.h"
#include "Amy_sdk.h"

static void call_handler(void* data) {
    /**
    * 收到的呼叫请求或者回复处理。
    * 本demo仅演示收到呼叫请求时的操作
    * -若外设处于空闲状态，可以建立起P2P会话，则回复answer
    * -否则，回复busy状态。
    *
    * 此回调建议处理所有的事件类型（参考枚举TUYA_TMM_CONTROL_EVT_E）
    */
    AMY_LOG_INFO("------------------->>>call event:%d\n", (int)data);
    if ((int)data == TUYA_TMM_CONTROL_EVT_INCOMING) {
        if (tuya_ipc_media_adapter_output_device_is_busy(0, 0) == FALSE) {
            tuya_tmm_control_answer();
        }
        else {
            tuya_tmm_control_busy();
        }
    }

}

static int tmm_control_evt_cb(TUYA_TMM_CONTROL_INFO_S* pinfo, VOID* priv_data) {
    /**
   * 收到呼叫请求或者回复的处理函数
   * 此处需要使用异步处理，否则会阻塞SDK内部线程的正常运转。
   * 客户可以自己创建线程或者工作队列来处理此异步请求，也可以使用涂鸦SDK内部的workq处理
   * 若使用涂鸦内部workq处理，此部分代码无需修改
  */
    AMY_LOG_INFO("receive evt %d\n", pinfo->event);
    WORKQUEUE_HANDLE ty_wq_hand = tal_workq_get_handle(WORKQ_SYSTEM);
    tal_workqueue_schedule(ty_wq_hand, call_handler, (VOID*)(pinfo->event));
    return 0;
}

int tuya_ipc_call_init(void) {
    return tuya_tmm_control_init(tmm_control_evt_cb, NULL, 30);
}