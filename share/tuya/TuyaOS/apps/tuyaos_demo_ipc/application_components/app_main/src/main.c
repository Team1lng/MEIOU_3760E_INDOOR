#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <signal.h>
#include <sys/prctl.h>

#include "utilities/uni_log.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_sdk_init.h"
#include "tuya_svc_netmgr_linkage.h"
#include "ty_sdk_common.h"
#include "ty_sdk_call.h"
#include "tuya_svc_netmgr_linkage.h"
#include "tal_system.h"

STATIC INT_T s_mode = -1;
STATIC CHAR_T s_token[30] = {0};

CHAR_T s_raw_path[128] = "/tmp/";

CHAR_T s_ipc_pid[64]="tuya_pid";//Product ID of TUYA device, this is for demo only.
CHAR_T s_ipc_uuid[64]="tuya_uuid";//Unique identification of each device//Contact tuya PM/BD for developing devices or BUY more
CHAR_T s_ipc_authkey[64]="tuya_authkey";//Authentication codes corresponding to UUID, one machine one code, paired with UUID.
CHAR_T s_ipc_storage[64]="/tmp/";//Path to save tuya sdk DB files, should be readable, writeable and storable
CHAR_T s_ipc_env[64]="";
CHAR_T s_ipc_sd_path[128]="/tmp/";//SD card mount directory
CHAR_T s_ipc_upgrade_file[128] = "/tmp/upgrade.file";//File with path to download file during OTA
CHAR_T s_app_version[64] = "1.2.3";//Firemware version displayed on TUYA APP

STATIC TUYA_IPC_SDK_RUN_VAR_S g_sdk_run_info = {0};

extern VOID TUYA_IPC_Status_Changed_cb(IN TUYA_IPC_STATUS_GROUP_E changed_group, IN CONST TUYA_IPC_STATUS_E status[TUYA_IPC_STATUS_GROUP_MAX]);
extern INT_T TUYA_IPC_sd_status_upload(INT_T status);
extern VOID TUYA_IPC_qrcode_shorturl_cb(CHAR_T* shorturl);
extern INT_T TUYA_IPC_Upgrade_Inform_cb(IN CONST FW_UG_S *fw);
extern VOID TUYA_IPC_Reset_System_CB(GW_RESET_TYPE_E type);
extern VOID TUYA_IPC_Restart_Process_CB(VOID);
extern INT_T TUYA_IPC_Get_MqttStatus();

extern INT_T TUYA_IPC_p2p_event_cb(IN CONST INT_T device, IN CONST INT_T channel, IN CONST MEDIA_STREAM_EVENT_E event, IN PVOID_T args);
extern VOID TUYA_IPC_APP_rev_audio_cb(IN INT_T device, IN INT_T channel, IN CONST MEDIA_AUDIO_FRAME_T *p_audio_frame);
extern VOID TUYA_IPC_APP_rev_video_cb(IN INT_T device, IN INT_T channel, IN CONST MEDIA_VIDEO_FRAME_T *p_video_frame);
extern VOID TUYA_IPC_APP_rev_file_cb(IN INT_T device, IN INT_T channel, IN CONST MEDIA_FILE_DATA_T *p_file_data);
extern VOID TUYA_APP_get_snapshot_cb(IN INT_T device, IN INT_T channel, OUT CHAR_T *snap_addr, OUT INT_T *snap_size);
extern VOID TUYA_APP_ai_result_cb(OUT CHAR_T *result_list, OUT CHAR_T *result_msg, OUT UINT_T msg_time, OUT UINT_T sn, OUT RULE_RESULT_STATE_E state);
extern VOID TUYA_APP_video_msg_cb(OUT CHAR_T *video_msg, OUT UINT_T msg_time, OUT UINT_T sn, OUT RULE_RESULT_STATE_E state);
extern VOID TUYA_IPC_Media_Adapter_Init(TUYA_IPC_SDK_MEDIA_ADAPTER_S *p_media_adatper_info, TUYA_IPC_SDK_MEDIA_STREAM_S *p_media_infos);
extern VOID TUYA_IPC_Media_Stream_Init(TUYA_IPC_SDK_MEDIA_ADAPTER_S *p_media_adatper_info);
extern OPERATE_RET TUYA_APP_Init_Ring_Buffer(CONST IPC_MEDIA_INFO_T *pMediaInfo, INT_T channel);
extern OPERATE_RET TUYA_APP_Put_Frame(RING_BUFFER_USER_HANDLE_T handle, IN CONST MEDIA_FRAME_T *p_frame);

extern VOID TUYA_IPC_upload_all_status(VOID);
extern VOID TUYA_IPC_handle_dp_cmd_objs(IN CONST TY_RECV_OBJ_DP_S *dp_rev);
extern VOID TUYA_IPC_handle_raw_dp_cmd_objs(IN CONST TY_RECV_RAW_DP_S *dp_rev);
extern VOID TUYA_IPC_handle_dp_query_objs(IN CONST TY_DP_QUERY_S *dp_query);
extern VOID TUYA_IPC_INIT_DP_CONFIG();

void signal_handle(int sig)
{
    char cThreadName[32] = {0};
    prctl(PR_GET_NAME, (unsigned long)cThreadName);
    printf("[%s, %d] get signal(%d)  name(%s)\n", __FUNCTION__, __LINE__, sig, cThreadName);

    switch(sig) {
    case SIGINT:
    case SIGKILL:
    case SIGTERM:
    case SIGSEGV:
    case SIGABRT:
        exit(1);
        break;

    default:
        break;
    }

    return;
}

static void __USAGE(char *app_name)
{
    printf("%s -m mode -t token -r raw path -h\n", (char *)basename(app_name));
    printf("\t m:  1-PARING_MODE_WIFI_AP 2-PARING_MODE_DEBUG 3-PARING_MODE_WIRED, refer to TUYA_IPC_PARING_MODE_E\n"
        "\t t: token get form qrcode info\n"
        "\t r: raw source file path\n"
        "\t h: help info\n");

    return;
}

static int __TUYA_IPC_DEMO_CFG_PARSE(char * cfg_path)
{
    char buffer[4096];
    int read_len = sizeof(buffer);
    int fd = -1;
    ty_cJSON *cfg_json = NULL;

    if (cfg_path == NULL) {
        printf("cfg_path is null\n");
        return -1;
    }
    
    // 从配置路径里读取相关的配置文件，整体的配置环境需要守护程序建立
    fd = open(cfg_path, 2);
    printf("detect auto test enviroment, load config from %s\n", cfg_path);    
    // 检查是打开文件成功。
    if (fd < 0){
        printf("open path %s failed,%d\n", cfg_path,fd);
        return -1;
    }
        
    // 读取文件内容
    memset(buffer, 0, sizeof(buffer));
    while (read_len == read(fd, buffer, sizeof(buffer)));
    close(fd);
    printf("buffer is %s\n",buffer);
        
    // 解析JSON
    cfg_json = ty_cJSON_Parse(buffer);
    if (NULL == cfg_json){
        printf("load configure failed\n");
        return OPRT_CJSON_PARSE_ERR;
    }
    ty_cJSON* env = ty_cJSON_GetObjectItem(cfg_json, "env");
    if (env) {
        strcpy(s_ipc_env, env->valuestring);
        printf("set env to %s\n", s_ipc_env);
        tuya_svc_mqtt_direct_set_env(s_ipc_env);
    }
    // pid
    ty_cJSON* json_pid = ty_cJSON_GetObjectItem(cfg_json, "pid");
    if (!ty_cJSON_IsString(json_pid)) {
        printf("get pid failed\n");
        ty_cJSON_Delete(cfg_json);
        return -1;
    }
    strcpy(s_ipc_pid, json_pid->valuestring);
    // uuid
    ty_cJSON* json_uuid = ty_cJSON_GetObjectItem(cfg_json, "uuid");
    if (!ty_cJSON_IsString(json_uuid)) {
        printf("get uuid failed\n");
        ty_cJSON_Delete(cfg_json);
        return -1;
    }
    strcpy(s_ipc_uuid, json_uuid->valuestring);
    // authkey
    ty_cJSON* json_authkey = ty_cJSON_GetObjectItem(cfg_json, "authkey");
    if (!ty_cJSON_IsString(json_authkey)) {
        printf("get authkey failed\n");
        ty_cJSON_Delete(cfg_json);
        return -1;
    }
    strcpy(s_ipc_authkey, json_authkey->valuestring);
    // storage path
    ty_cJSON* json_storage = ty_cJSON_GetObjectItem(cfg_json, "storage");
    if (ty_cJSON_IsString(json_storage)) {
        strcpy(s_ipc_storage, json_storage->valuestring);
    }
    // media path
    ty_cJSON* json_media = ty_cJSON_GetObjectItem(cfg_json, "media_path");
    if (ty_cJSON_IsString(json_media)) {
        strcpy(s_raw_path, json_media->valuestring);
    }
    // sd path
    ty_cJSON* json_sd_path = ty_cJSON_GetObjectItem(cfg_json, "sd_path");
    if (ty_cJSON_IsString(json_sd_path)) {
        strcpy(s_ipc_sd_path, json_sd_path->valuestring);
    }

    // upgrade file
    ty_cJSON* json_upgrade_file = ty_cJSON_GetObjectItem(cfg_json, "upgrade_file");
    if (ty_cJSON_IsString(json_upgrade_file)) {
        strcpy(s_ipc_upgrade_file, json_upgrade_file->valuestring);
    }
    // app version
    ty_cJSON* json_app_version = ty_cJSON_GetObjectItem(cfg_json, "app_version");
    if (ty_cJSON_IsString(json_app_version)) {
        strcpy(s_app_version, json_app_version->valuestring);
    }

    // token
    ty_cJSON* json_token = ty_cJSON_GetObjectItem(cfg_json, "token");
    if (ty_cJSON_IsString(json_token)) {
        strcpy(s_token, json_token->valuestring);
    }
    // mode
    ty_cJSON* json_mode = ty_cJSON_GetObjectItem(cfg_json, "mode");
    if (ty_cJSON_IsString(json_mode)) {
        s_mode = atoi(json_mode->valuestring);
    }
    ty_cJSON_Delete(cfg_json);

    printf("s_ipc_pid: %s\n", s_ipc_pid);
    printf("s_ipc_uuid: %s\n", s_ipc_uuid);
    printf("s_ipc_authkey: %s\n", s_ipc_authkey);
    printf("s_ipc_storage: %s\n", s_ipc_storage);
    printf("s_raw_path: %s\n", s_raw_path);
    printf("s_ipc_sd_path: %s\n", s_ipc_sd_path);
    printf("s_ipc_upgrade_file: %s\n", s_ipc_upgrade_file);
    printf("s_app_version: %s\n", s_app_version);
    
    return 0;
}

VOID TUYA_IPC_simulation()
{
    /* Manual simulation of related functions */
    char test_input[64] = {0};
    int ret = 0;
    while(1)
    {
        scanf("%s",test_input);
        /* Simulation of the start of motion detection events */
        if(0 == strcmp(test_input,"md_start"))
        {
            IPC_APP_set_motion_status(1);
        }
        /* Simulation of the end of event */
        else if(0 == strcmp(test_input,"md_stop"))
        {
            IPC_APP_set_motion_status(0);
        }
        /* Simulation of getting device's activation status */
        else if(0 == strcmp(test_input,"status"))
        {
            IPC_REGISTER_STATUS_E status = tuya_ipc_get_register_status();
            PR_INFO("current register status %d[0:unregistered 1:registered 2:activated]\n",status);
        }
        /* Simulation of doorbell press event */
        else if(0 == strcmp(test_input,"bell_press"))
        {
            //Using demo file for simulation, should be replaced by real snapshot when events happen.
            int snapshot_size = 150*1024;
            char *snapshot_buf = (char *)malloc(snapshot_size);
            int ret = IPC_APP_get_snapshot(snapshot_buf, &snapshot_size);
            if(ret != 0)
            {
                PR_ERR("Get snap fail!\n");
                free(snapshot_buf);
                continue;
            }
            /* Push the detection message and the current snapshot image to the APP.
            Snapshot image acquisition needs to be implemented by the developer */
            tuya_ipc_door_bell_press(DOORBELL_NORMAL, snapshot_buf, snapshot_size, NOTIFICATION_CONTENT_JPEG);
            free(snapshot_buf);
        }
        /* Simulation of low power ipc */
        else if (0 == strcmp(test_input, "start_low_power"))
        {
            TUYA_IPC_low_power_sample();
        }
        else if (0 == strcmp(test_input, "bell_ac"))
        {
            doorbell_handler();
        }
        /* Simulation of get time for OSD */
        else if (0 == strcmp(test_input, "osd"))
        {
            struct tm localTime;
            tuya_ipc_get_tm_with_timezone_dls(&localTime);
            PR_DEBUG("show OSD [%04d-%02d-%02d %02d:%02d:%02d]",localTime.tm_year,localTime.tm_mon,localTime.tm_mday,localTime.tm_hour,localTime.tm_min,localTime.tm_sec);
        }
        else if (0 == strcmp(test_input, "album_eme")) {
            ret = tuya_ipc_ss_set_write_mode(1);
            PR_DEBUG("tuya_ipc_ss_set_write_mode is %d\n",ret);
            album_file_put();       // copy demo video to sdcard for view on the app
        }
#if defined(ENABLE_TUYA_CALL)&&(ENABLE_TUYA_CALL==1)
        else if(0 == strcmp(test_input, "call")) 
        {
            TUYA_IPC_call_app();
        }
        else if(0 == strcmp(test_input, "hangup"))
        {
            TUYA_IPC_hangup();
        }
        else if(0 == strcmp(test_input, "download"))
        {
            TUYA_IPC_download_background_demo();
        } 
#endif
#if defined(ENABLE_AOV_STORAGE)&&(ENABLE_AOV_STORAGE==1)
        else if(0 == strcmp(test_input, "aov_video_test"))
        {
            ty_aov_video_test();
        } 
#endif
        tal_system_sleep(100);
    }

    return;
}

static OPERATE_RET tuya_ipc_app_start(IN TUYA_IPC_SDK_RUN_VAR_S * pRunInfo)
{
	if(NULL == pRunInfo)
	{
		PR_ERR("start sdk para is NULL\n");
		return OPRT_INVALID_PARM;
	}

    OPERATE_RET ret = 0;
    STATIC BOOL_T s_ipc_sdk_started = FALSE;
    if(TRUE == s_ipc_sdk_started )
    {
        PR_DEBUG("IPC SDK has started\n");
        return ret;
    }

	memcpy(&g_sdk_run_info, pRunInfo, SIZEOF(TUYA_IPC_SDK_RUN_VAR_S));

	//setup1:init sdk
    TUYA_IPC_ENV_VAR_T env;
    memset(&env, 0, sizeof(TUYA_IPC_ENV_VAR_T));
    strcpy(env.storage_path, pRunInfo->iot_info.cfg_storage_path);
    strcpy(env.product_key,pRunInfo->iot_info.product_key);
    strcpy(env.uuid, pRunInfo->iot_info.uuid);
    strcpy(env.auth_key, pRunInfo->iot_info.auth_key);
    strcpy(env.dev_sw_version, pRunInfo->iot_info.dev_sw_version);
    strcpy(env.dev_serial_num, "tuya_ipc");
    //TODO:raw
    env.dev_raw_dp_cb = pRunInfo->dp_info.raw_dp_cmd_proc;
    env.dev_obj_dp_cb = pRunInfo->dp_info.common_dp_cmd_proc;
    env.dev_dp_query_cb = pRunInfo->dp_info.dp_query;
    env.status_changed_cb = pRunInfo->net_info.ipc_status_change_cb;
    env.upgrade_cb_info.upgrade_cb = pRunInfo->upgrade_info.upgrade_cb;
    env.gw_rst_cb = pRunInfo->iot_info.gw_reset_cb;
    env.gw_restart_cb = pRunInfo->iot_info.gw_restart_cb;
    env.qrcode_active_cb = pRunInfo->qrcode_active_cb;
    env.dev_type = pRunInfo->iot_info.dev_type;
    env.link_type = pRunInfo->net_info.link_type;
    env.ip_mode_type = pRunInfo->net_info.ip_mode_type;
    ret = tuya_ipc_init_sdk(&env);
	if(OPRT_OK != ret)
	{
		PR_ERR("init sdk is error\n");
		return ret;
	}

	//设置日志等级
	tuya_ipc_set_log_attr(pRunInfo->debug_info.log_level,NULL);
	
    //setup2: set media adapter
    TUYA_IPC_Media_Adapter_Init(&pRunInfo->media_adatper_info, &pRunInfo->media_info);

	//setup 3: ring buffer 创建。
	ret = TUYA_APP_Init_Ring_Buffer(&pRunInfo->media_info.media_info, 0);
	if(OPRT_OK != ret)
	{
		PR_ERR("create ring buffer is error\n");
		return ret;
	}
    tuya_ipc_ring_buffer_adapter_register_media_source();
    #if 0
	//低功耗 优先开启P2P
    if(g_sdk_run_info.quick_start_info.enable)
    {
        pthread_t low_power_p2p_thread_handler;
        int op_ret = pthread_create(&low_power_p2p_thread_handler,NULL, tuya_ipc_sdk_low_power_p2p_init_proc, NULL);
        if(op_ret < 0)
        {
            PR_ERR("create p2p start thread is error\n");
            return -1;
        }
    }
	#endif
	ret = tuya_ipc_start_sdk(pRunInfo->net_info.connect_mode,pRunInfo->debug_info.qrcode_token);
	if(OPRT_OK != ret)
	{
		PR_ERR("start sdk is error\n");
		return ret;
	}

	s_ipc_sdk_started = true;
	PR_DEBUG("tuya ipc sdk start is complete\n");
	return ret;
}

static OPERATE_RET __TUYA_IPC_SDK_START(TUYA_IPC_PARING_MODE_E connect_mode, CHAR_T *p_token)
{
    printf("SDK Version:%s\r\n", tuya_ipc_get_sdk_info());
    TUYA_IPC_LINK_TYPE_E link_type = TUYA_IPC_LINK_WIFI;
	TUYA_IPC_SDK_RUN_VAR_S ipc_sdk_run_var ={0};
	memset(&ipc_sdk_run_var,0,sizeof(ipc_sdk_run_var));

	/*certification information(essential)*/
	strcpy(ipc_sdk_run_var.iot_info.product_key,s_ipc_pid);
	strcpy(ipc_sdk_run_var.iot_info.uuid,s_ipc_uuid);
	strcpy(ipc_sdk_run_var.iot_info.auth_key,s_ipc_authkey);
	strcpy(ipc_sdk_run_var.iot_info.dev_sw_version,s_app_version);
	strcpy(ipc_sdk_run_var.iot_info.cfg_storage_path,s_ipc_storage);
	//normal device
	ipc_sdk_run_var.iot_info.dev_type = NORMAL_POWER_DEV;
	//if needed, change to low power device
	//ipc_sdk_run_var.iot_info.dev_type= LOW_POWER_DEV;

	/*connect mode (essential)*/
	ipc_sdk_run_var.net_info.connect_mode = connect_mode;
	ipc_sdk_run_var.net_info.ipc_status_change_cb = TUYA_IPC_Status_Changed_cb;
    ipc_sdk_run_var.net_info.link_type = link_type;
    ipc_sdk_run_var.net_info.ip_mode_type = TUYA_IPC_IPV4_ONLY;
    printf("MODE:%d  LINK_TYPE:%d IP_MODE_TYPE:%d\r\n", connect_mode, ipc_sdk_run_var.net_info.link_type, ipc_sdk_run_var.net_info.ip_mode_type);
	if(p_token)
	{
	    strcpy(ipc_sdk_run_var.debug_info.qrcode_token,p_token);
	}
	/* 0-5, the bigger, the more log */
	ipc_sdk_run_var.debug_info.log_level = 4;
	/*media info (essential)*/
    /* main stream(HD), video configuration*/
    /* NOTE
    FIRST:If the main stream supports multiple video stream configurations, set each item to the upper limit of the allowed configuration.
    SECOND:E_IPC_STREAM_VIDEO_MAIN must exist.It is the data source of SDK.
    please close the E_IPC_STREAM_VIDEO_SUB for only one stream*/
    ipc_sdk_run_var.media_info.media_info.stream_enable[E_IPC_STREAM_VIDEO_MAIN] = TRUE;    /* Whether to enable local HD video streaming */
    ipc_sdk_run_var.media_info.media_info.video_fps[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* FPS */
    ipc_sdk_run_var.media_info.media_info.video_gop[E_IPC_STREAM_VIDEO_MAIN] = 30;  /* GOP */
    ipc_sdk_run_var.media_info.media_info.video_bitrate[E_IPC_STREAM_VIDEO_MAIN] = TUYA_VIDEO_BITRATE_1M; /* Rate limit */
    ipc_sdk_run_var.media_info.media_info.video_width[E_IPC_STREAM_VIDEO_MAIN] = 640; /* Single frame resolution of width*/
    ipc_sdk_run_var.media_info.media_info.video_height[E_IPC_STREAM_VIDEO_MAIN] = 360;/* Single frame resolution of height */
    ipc_sdk_run_var.media_info.media_info.video_freq[E_IPC_STREAM_VIDEO_MAIN] = 90000; /* Clock frequency */
    ipc_sdk_run_var.media_info.media_info.video_codec[E_IPC_STREAM_VIDEO_MAIN] = TUYA_CODEC_VIDEO_H264; /* Encoding format */

    /* substream(HD), video configuration */
    /* Please note that if the substream supports multiple video stream configurations, please set each item to the upper limit of the allowed configuration. */
    ipc_sdk_run_var.media_info.media_info.stream_enable[E_IPC_STREAM_VIDEO_SUB] = TRUE;     /* Whether to enable local SD video stream */
    ipc_sdk_run_var.media_info.media_info.video_fps[E_IPC_STREAM_VIDEO_SUB] = 30;  /* FPS */
    ipc_sdk_run_var.media_info.media_info.video_gop[E_IPC_STREAM_VIDEO_SUB] = 30;  /* GOP */
    ipc_sdk_run_var.media_info.media_info.video_bitrate[E_IPC_STREAM_VIDEO_SUB] = TUYA_VIDEO_BITRATE_512K; /* Rate limit */
    ipc_sdk_run_var.media_info.media_info.video_width[E_IPC_STREAM_VIDEO_SUB] = 640; /* Single frame resolution of width */
    ipc_sdk_run_var.media_info.media_info.video_height[E_IPC_STREAM_VIDEO_SUB] = 360;/* Single frame resolution of height */
    ipc_sdk_run_var.media_info.media_info.video_freq[E_IPC_STREAM_VIDEO_SUB] = 90000; /* Clock frequency */
    ipc_sdk_run_var.media_info.media_info.video_codec[E_IPC_STREAM_VIDEO_SUB] = TUYA_CODEC_VIDEO_H264; /* Encoding format */

    /* Audio stream configuration.
    Note: The internal P2P preview, cloud storage, and local storage of the SDK are all use E_IPC_STREAM_AUDIO_MAIN data. */
    ipc_sdk_run_var.media_info.media_info.stream_enable[E_IPC_STREAM_AUDIO_MAIN] = TRUE;         /* Whether to enable local sound collection */
    ipc_sdk_run_var.media_info.media_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN] = TUYA_CODEC_AUDIO_PCM;/* Encoding format */
    ipc_sdk_run_var.media_info.media_info.audio_sample [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_SAMPLE_8K;/* Sampling Rate */
    ipc_sdk_run_var.media_info.media_info.audio_databits [E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_DATABITS_16;/* Bit width */
    ipc_sdk_run_var.media_info.media_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN]= TUYA_AUDIO_CHANNEL_MONO;/* channel */
    ipc_sdk_run_var.media_info.media_info.audio_fps[E_IPC_STREAM_AUDIO_MAIN] = 25;/* Fragments per second */

    /*local storage (custome whether enable or not)*/
    ipc_sdk_run_var.local_storage_info.enable = 1;
    ipc_sdk_run_var.local_storage_info.max_event_num_per_day = 500;
    ipc_sdk_run_var.local_storage_info.skills = 0;//0 means default skills   (TUYA_IPC_SKILL_BASIC | TUYA_IPC_SKILL_DELETE_BY_DAY | TUYA_IPC_SKILL_SPEED_PLAY_0Point5 | TUYA_IPC_SKILL_SPEED_PLAY_2 | TUYA_IPC_SKILL_SPEED_PLAY_4 | TUYA_IPC_SKILL_SPEED_PLAY_8)
    //eg support local download
    //ipc_sdk_run_var.local_storage_info.skills = TUYA_IPC_SKILL_DOWNLOAD | TUYA_IPC_SKILL_BASIC | TUYA_IPC_SKILL_DELETE_BY_DAY | TUYA_IPC_SKILL_SPEED_PLAY_0Point5 | TUYA_IPC_SKILL_SPEED_PLAY_2 | TUYA_IPC_SKILL_SPEED_PLAY_4 | TUYA_IPC_SKILL_SPEED_PLAY_8 ;
    ipc_sdk_run_var.local_storage_info.sd_status_cb = TUYA_IPC_sd_status_upload ;
	strcpy(ipc_sdk_run_var.local_storage_info.storage_path, s_ipc_sd_path);

    /*cloud_storage_info*/
    ipc_sdk_run_var.cloud_storage_info.enable = 1;
    ipc_sdk_run_var.cloud_storage_info.en_audio_record = 1;
    ipc_sdk_run_var.cloud_storage_info.pre_record_time = 2;

	/*media adapter function (essential)*/
	ipc_sdk_run_var.media_adatper_info.max_stream_client = 5;
	ipc_sdk_run_var.media_adatper_info.live_mode = TRANS_DEFAULT_STANDARD;
	ipc_sdk_run_var.media_adatper_info.media_event_cb = TUYA_IPC_p2p_event_cb;
	ipc_sdk_run_var.media_adatper_info.rev_audio_cb = TUYA_IPC_APP_rev_audio_cb;
    ipc_sdk_run_var.media_adatper_info.rev_video_cb = TUYA_IPC_APP_rev_video_cb;
    ipc_sdk_run_var.media_adatper_info.rev_file_cb = TUYA_IPC_APP_rev_file_cb;
    ipc_sdk_run_var.media_adatper_info.get_snapshot_cb = TUYA_APP_get_snapshot_cb;

	/*event function (essential)*/
	TUYA_ALARM_BITMAP_T alarm_of_events[1] = {0};
	tuya_ipc_event_add_alarm_types(&alarm_of_events[0], E_ALARM_MOTION);
	ipc_sdk_run_var.event_info.enable = 1;
	ipc_sdk_run_var.event_info.max_event = sizeof(alarm_of_events) / sizeof(TUYA_ALARM_BITMAP_T);
	ipc_sdk_run_var.event_info.alarms_of_events = alarm_of_events;
	ipc_sdk_run_var.event_info.alarms_cnt_per_event = 1;

	/*AI detect (custome whether enable or not)*/
	ipc_sdk_run_var.cloud_rule_info.enable = 1;
	ipc_sdk_run_var.cloud_rule_info.on_ai_result_cb = TUYA_APP_ai_result_cb;
	ipc_sdk_run_var.cloud_rule_info.on_video_msg_cb = TUYA_APP_video_msg_cb;

	/*door bell (custome whether enable or not)*/
	ipc_sdk_run_var.video_msg_info.enable = 1;
	ipc_sdk_run_var.video_msg_info.type = MSG_BOTH;
	ipc_sdk_run_var.video_msg_info.msg_duration = 10;

	/*dp function(essential)*/
	ipc_sdk_run_var.dp_info.dp_query = TUYA_IPC_handle_dp_query_objs;
	ipc_sdk_run_var.dp_info.raw_dp_cmd_proc = TUYA_IPC_handle_raw_dp_cmd_objs;
	ipc_sdk_run_var.dp_info.common_dp_cmd_proc = TUYA_IPC_handle_dp_cmd_objs;

	/*upgrade function(essential)*/
	ipc_sdk_run_var.upgrade_info.enable = true;
	ipc_sdk_run_var.upgrade_info.upgrade_cb = TUYA_IPC_Upgrade_Inform_cb;
	strcpy(ipc_sdk_run_var.upgrade_info.upgrade_file, s_ipc_upgrade_file);

	ipc_sdk_run_var.iot_info.gw_reset_cb = TUYA_IPC_Reset_System_CB;
	ipc_sdk_run_var.iot_info.gw_restart_cb = TUYA_IPC_Restart_Process_CB;

    /*QR-active function(essential)*/
    ipc_sdk_run_var.qrcode_active_cb = NULL;//TUYA_IPC_qrcode_shorturl_cb;

    /* rtc call function(essential), only use in two-way video talk */
    ipc_sdk_run_var.call_info.enable = FALSE;

	OPERATE_RET ret ;
    ret = tuya_ipc_app_start(&ipc_sdk_run_var);
    if(ret !=0 )
    {
    	PR_DEBUG("ipc sdk start fail,please check run parameter，ret=%d\n",ret);
    }
    if (TUYA_IPC_LINK_WIRE == link_type){
        //if only use wire for paring
        tuya_svc_netmgr_linkage_set_default(LINKAGE_TYPE_WIRED);
    }
	return ret;
}



static OPERATE_RET TUYA_APP_Init_Stream_Storage(TUYA_IPC_SDK_LOCAL_STORAGE_S * p_local_storage_info)
{
    STATIC BOOL_T s_stream_storage_inited = FALSE;
    char * mount_path = tuya_ipc_get_sd_mount_path();

    if(s_stream_storage_inited == TRUE)
    {
        PR_DEBUG("The Stream Storage Is Already Inited");
        return OPRT_OK;
    }

    if(p_local_storage_info == NULL)
    {
        PR_DEBUG("Init Stream Storage fail. Param is null");
        return OPRT_INVALID_PARM;
    }
    TUYA_IPC_STORAGE_VAR_T stg_var;
    memset(&stg_var, 0, SIZEOF(TUYA_IPC_STORAGE_VAR_T));
    memcpy(stg_var.base_path, p_local_storage_info->storage_path, SS_BASE_PATH_LEN);
    strncpy(mount_path, p_local_storage_info->storage_path, sizeof(p_local_storage_info->storage_path)-1);
    stg_var.max_event_per_day = p_local_storage_info->max_event_num_per_day;
    stg_var.sd_status_changed_cb = p_local_storage_info->sd_status_cb;
    stg_var.skills = p_local_storage_info->skills;

    stg_var.aov_info.aov_stor_enable = 0;   // AOV local storage default off
    stg_var.aov_info.enable_channel = E_IPC_STREAM_VIDEO_MAIN;
    stg_var.aov_info.aov_video_frame_duration_ms = 5000;    // AOV 视频帧间隔时长(ms)
    stg_var.aov_info.aov_video_bitrate = 1500;              // AOV 码率
    stg_var.aov_info.aov_video_gop = 40;
    stg_var.aov_info.aov_video_width = 2560;                // 根据实际分辨率填写
    stg_var.aov_info.aov_video_height = 1440;
    stg_var.aov_info.aov_video_codec = TUYA_CODEC_VIDEO_H265;

    PR_DEBUG("Init Stream_Storage SD:%s", p_local_storage_info->storage_path);
    OPERATE_RET ret = tuya_ipc_ss_init(&stg_var);
    if(ret != OPRT_OK)
    {
        PR_ERR("Init Main Video Stream_Storage Fail. %d", ret);
        return OPRT_COM_ERROR;
    }

#if 0
    // 记录仪&相册组件如有需要，可单独初始化
    TUYA_IPC_ALBUM_INFO_T album_info;
    memset(&album_info, 0, sizeof(album_info));
    album_info.cnt = 1;
    memcpy(&album_info.album_name[0], TUYA_IPC_ALBUM_EMERAGE_FILE, strlen(TUYA_IPC_ALBUM_EMERAGE_FILE));
    
    TUYA_IPC_ALBUM_WORKING_PATH_NAME_T album_path;
    memset(&album_path, 0, sizeof(album_path));
    memcpy(album_path.mount_path, p_local_storage_info->storage_path, SS_BASE_PATH_LEN);
    ret = tuya_ipc_album_ss_init(&album_info, &album_path, &g_sdk_run_info.media_info.media_info);
    PR_DEBUG("Init album :%d", ret);
#endif

    return OPRT_OK;
}

#if defined(ENABLE_CLOUD_RULE)&&(ENABLE_CLOUD_RULE==1)
static OPERATE_RET TUYA_APP_Init_Cloud_Rule(TUYA_IPC_SDK_CLOUD_RULE_S * p_cloud_rule)
{
    tuya_ipc_rule_init((RULE_AI_RESULT_CB)p_cloud_rule->on_ai_result_cb, (RULE_VIDEO_MSG_CB)p_cloud_rule->on_video_msg_cb);
}
#endif

static OPERATE_RET TUYA_APP_Enable_CloudStorage(TUYA_IPC_SDK_CLOUD_STORAGE_S *p_cloud_storage_info)
{
    OPERATE_RET ret;
    ret = tuya_ipc_cloud_storage_init();
    if(ret != OPRT_OK)
    {
        PR_DEBUG("Cloud Storage Init Err! ret :%d", ret);
        return ret;
    }

    if(p_cloud_storage_info->en_audio_record == FALSE)
    {
        tuya_ipc_cloud_storage_set_audio_stat(p_cloud_storage_info->en_audio_record);
        PR_DEBUG("Disable audio record");
    }

    // Set pre-record time ,if needed. default pre-record time:2 seconds
    if(p_cloud_storage_info->pre_record_time >= 0)
    {
        ret = tuya_ipc_cloud_storage_set_pre_record_time(p_cloud_storage_info->pre_record_time);
        PR_DEBUG("Set pre-record time to [%d], [%s]", p_cloud_storage_info->pre_record_time, ret == OPRT_OK ? "success" : "failure");
    }
    return OPRT_OK;
}

#if defined(OPERATING_SYSTEM) && (OPERATING_SYSTEM == 100)//linux
int main(int argc, char** argv)
{
    INT_T res = -1;
    TUYA_IPC_PARING_MODE_E mode = PARING_MODE_DEBUG;
    char  cfg_path[256] = {0};

    while((res = getopt(argc, argv, "?m:t:s:r:c:h")) != -1)
    {
        switch(res) {
        case 'm':
            mode = atoi(optarg);
            break;
        case 't':
            strcpy(s_token, optarg);
            break;
        case 'r':
            strcpy(s_raw_path, optarg);
            break;
        case 'c':
            snprintf(cfg_path, 256, "%s/tuya_device_cfg.json", optarg);
            break;
        case 'h':
        default:
            __USAGE(argv[0]);
            return -1;
        }
    }
    
    printf("[%s, %d] %s,%s token[%s] raw_path[%s] cfg_path[%s]\n", __FUNCTION__, __LINE__, __DATE__, __TIME__,s_token, s_raw_path, cfg_path);

    if (0 != __TUYA_IPC_DEMO_CFG_PARSE(cfg_path))
    {
        printf("media cfg parse failed\n");
        return -1;
    }
    TUYA_IPC_INIT_DP_CONFIG();

    signal(SIGINT, signal_handle);
    signal(SIGKILL, signal_handle);
    signal(SIGTERM, signal_handle);
    signal(SIGPIPE, SIG_IGN);

    res = __TUYA_IPC_SDK_START(mode,s_token);
    if(res != OPRT_OK){
        return res;
    }

    TUYA_APP_Enable_Motion_Detect();

#ifdef ENABLE_SHADOW_DEVICE
    rpc_set_buffer_size(65535);
    rpc_server_init(tuya_ipc_rpc_server_func_call_cb);
    tuya_ipc_rpc_server_declare_all();
#endif
	
    /* whether SDK is connected to MQTT */
    while(TUYA_IPC_Get_MqttStatus() == FALSE)
    {
        tal_system_sleep(1000);
    }
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is start run\n");
    //同步服务器时间
    TIME_T time_utc;
    INT_T time_zone;
    do
    {
        //需要SDK同步到时间后才能开启下面的业务
        res = tuya_ipc_get_service_time_force(&time_utc, &time_zone);

    } while(res != OPRT_OK);

    TUYA_IPC_av_start();

    if(FALSE == g_sdk_run_info.quick_start_info.enable)
    {
        TUYA_IPC_Media_Stream_Init(&(g_sdk_run_info.media_adatper_info));
    }

    if(g_sdk_run_info.event_info.enable)
    {
        res = tuya_ipc_event_module_init(g_sdk_run_info.event_info.max_event, 
                            g_sdk_run_info.event_info.alarms_of_events, g_sdk_run_info.event_info.alarms_cnt_per_event);
        PR_DEBUG("event module init result is %d\n",res);
    }

    if(g_sdk_run_info.local_storage_info.enable)
    {
        res = TUYA_APP_Init_Stream_Storage(&(g_sdk_run_info.local_storage_info));
        PR_DEBUG("local storage init result is %d\n",res);
    }

    if(g_sdk_run_info.video_msg_info.enable)
    {
        res =  TUYA_APP_Enable_Video_Msg(&(g_sdk_run_info.video_msg_info));
        PR_DEBUG("door bell init result is %d\n",res);
    }
#if defined(ENABLE_CLOUD_RULE)&&(ENABLE_CLOUD_RULE==1)
    if(g_sdk_run_info.cloud_rule_info.enable){
        res = TUYA_APP_Init_Cloud_Rule(&(g_sdk_run_info.cloud_rule_info));
        PR_DEBUG("cloud rule init result is %d\n",res);
    }
#endif
    if(g_sdk_run_info.cloud_storage_info.enable)
    {
        res = TUYA_APP_Enable_CloudStorage(&(g_sdk_run_info.cloud_storage_info));
        PR_DEBUG("cloud storage init result is %d\n",res);
    }
#if defined(ENABLE_TUYA_CALL)&&(ENABLE_TUYA_CALL==1)
    if(g_sdk_run_info.call_info.enable) 
    {
        TUYA_IPC_call_init();   
    }
#endif
    TUYA_IPC_upload_all_status();

    tuya_ipc_upload_skills();
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is end run\n");

    TUYA_IPC_simulation();
    
    printf("[%s, %d]\n", __FUNCTION__, __LINE__);

    return 0;
}

#else //non linux
void user_main(void)
{
    INT_T res = -1;
    TUYA_IPC_PARING_MODE_E mode = PARING_MODE_DEBUG;
    char cfg_path[256] = {0};

    //Please ensure that the parameters have been set correctly before running
    strcpy(s_token, "xxxxxx"); //token get form qrcode info
    mode = 2; // 1-PARING_MODE_WIFI_AP 2-PARING_MODE_DEBUG 3-PARING_MODE_WIRED, refer to TUYA_IPC_PARING_MODE_E
    strcpy(s_raw_path, "xxxxxx"); //raw source file path

    printf("[%s, %d] %s,%s token[%s] raw_path[%s] cfg_path[%s]\n", __FUNCTION__, __LINE__, __DATE__, __TIME__,s_token, s_raw_path, cfg_path);

    if (0 != __TUYA_IPC_DEMO_CFG_PARSE(cfg_path))
    {
        printf("media cfg parse failed\n");
        return -1;
    }
    TUYA_IPC_INIT_DP_CONFIG();

    signal(SIGINT, signal_handle);
    signal(SIGKILL, signal_handle);
    signal(SIGTERM, signal_handle);
    signal(SIGPIPE, SIG_IGN);

    res = __TUYA_IPC_SDK_START(mode,s_token);
    if(res != OPRT_OK){
        return res;
    }

    TUYA_APP_Enable_Motion_Detect();

#ifdef ENABLE_SHADOW_DEVICE
    rpc_set_buffer_size(65535);
    rpc_server_init(tuya_ipc_rpc_server_func_call_cb);
    tuya_ipc_rpc_server_declare_all();
#endif
	
    /* whether SDK is connected to MQTT */
    while(TUYA_IPC_Get_MqttStatus() == FALSE)
    {
        tal_system_sleep(1000);
    }
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is start run\n");
    //同步服务器时间
    TIME_T time_utc;
    INT_T time_zone;
    do
    {
        //需要SDK同步到时间后才能开启下面的业务
        res = tuya_ipc_get_service_time_force(&time_utc, &time_zone);

    } while(res != OPRT_OK);

    TUYA_IPC_av_start();

    if(FALSE == g_sdk_run_info.quick_start_info.enable)
    {
        TUYA_IPC_Media_Stream_Init(&(g_sdk_run_info.media_adatper_info));
    }

    if(g_sdk_run_info.event_info.enable)
    {
        res = tuya_ipc_event_module_init(g_sdk_run_info.event_info.max_event, 
                            g_sdk_run_info.event_info.alarms_of_events, g_sdk_run_info.event_info.alarms_cnt_per_event);
        PR_DEBUG("event module init result is %d\n",res);
    }

    if(g_sdk_run_info.local_storage_info.enable)
    {
        res = TUYA_APP_Init_Stream_Storage(&(g_sdk_run_info.local_storage_info));
        PR_DEBUG("local storage init result is %d\n",res);
    }

    if(g_sdk_run_info.video_msg_info.enable)
    {
        res =  TUYA_APP_Enable_Video_Msg(&(g_sdk_run_info.video_msg_info));
        PR_DEBUG("door bell init result is %d\n",res);
    }
#if defined(ENABLE_CLOUD_RULE)&&(ENABLE_CLOUD_RULE==1)
    if(g_sdk_run_info.cloud_rule_info.enable){
        res = TUYA_APP_Init_Cloud_Rule(&(g_sdk_run_info.cloud_rule_info));
        PR_DEBUG("cloud rule init result is %d\n",res);
    }
#endif
    if(g_sdk_run_info.cloud_storage_info.enable)
    {
        res = TUYA_APP_Enable_CloudStorage(&(g_sdk_run_info.cloud_storage_info));
        PR_DEBUG("cloud storage init result is %d\n",res);
    }
#if defined(ENABLE_TUYA_CALL)&&(ENABLE_TUYA_CALL==1)
    if(g_sdk_run_info.call_info.enable) 
    {
        TUYA_IPC_call_init();   
    }
#endif
    TUYA_IPC_upload_all_status();

    tuya_ipc_upload_skills();
    PR_DEBUG("tuya_ipc_sdk_mqtt_online_proc is end run\n");

    TUYA_IPC_simulation();
    
    printf("[%s, %d]\n", __FUNCTION__, __LINE__);

    return 0;
}
#endif
