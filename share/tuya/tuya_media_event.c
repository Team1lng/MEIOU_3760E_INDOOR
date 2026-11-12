#include "tuya_sdk.h"
#include "tuya_ipc_event.h"
#include "tuya_ipc_sdk_init.h"
#include "tuya_ipc_upgrade.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_ipc_media_adapter.h"
#include "tuya_ipc_media_stream.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_sdk_init.h"
#include "Amy_tfcard.h"
#include "Amy_media_mgr.h"
#include "Amy_log.h"
#include "Amy_sdk.h"
#include "avilib/avilib.h"
typedef int (*media_event_cb_func)(void *arg);

#define PB_CLIENT_MAX (4)
typedef struct
{
    MEDIA_STREAM_EVENT_E ev;
    media_event_cb_func cb;
} media_event_cb_t;

typedef struct
{
    unsigned int ch;
    unsigned int id;
    int time_zone;
    unsigned int start_timestamp;
    unsigned int current_timestamp;
    unsigned int duration;
    unsigned int later_time;
    int pause; // 0:正常播放，1

    unsigned long video_frame_index;
    unsigned long oneframe_duration;
    unsigned char *audio_buf;
    int audio_buf_size;
    avi_t *avi_dev;

    int run;
    Amy_thread_t tid;
} pb_context;

static pb_context pb_ctx[PB_CLIENT_MAX] = {
    {.avi_dev = NULL, .pause = 0, .run = 0},
    {.avi_dev = NULL, .pause = 0, .run = 0},
    {.avi_dev = NULL, .pause = 0, .run = 0},
    {.avi_dev = NULL, .pause = 0, .run = 0},
};

static int media_stream_video_start_cb(void *arg)
{
    tuya_cb_attr attr = {0};
    attr.event = TUYA_EVENT_CMD_VIDEO_START;
    Amy_tuya_event_put(&attr);
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_video_stop_cb(void *arg)
{
    tuya_cb_attr attr = {0};
    attr.event = TUYA_EVENT_CMD_VIDEO_STOP;
    Amy_tuya_event_put(&attr);
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_audio_start_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_audio_stop_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_speaker_start_cb(void *arg)
{
    tuya_cb_attr attr = {0};
    attr.event = TUYA_EVENT_CMD_AUDIO_START;
    Amy_tuya_event_put(&attr);
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_speaker_stop_cb(void *arg)
{
    tuya_cb_attr attr = {0};
    attr.event = TUYA_EVENT_CMD_AUDIO_STOP;
    Amy_tuya_event_put(&attr);
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_display_start_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_display_stop_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_video_send_pause_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_video_send_resume_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_load_adjust_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_playback_load_adust_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_ability_query_cb(void *arg)
{

    AMY_LOG_ERROR("<<<-----------------\n");

    return 0;
}

// 查询指定年月下有多少天是有记录数据
static int media_stream_playback_query_month_simplify_cb(void *arg)
{
    C2C_TRANS_QUERY_PB_MONTH_INNER_REQ *p = (C2C_TRANS_QUERY_PB_MONTH_INNER_REQ *)arg;
    if (!p)
    {
        return 0;
    }
    AMY_LOG_INFO("tuya playback query (%d-%d)\n", p->year, p->month);

    Amy_media_context *ctx = Amy_media_open(TFCARD_MOUNT_PATH, MEDIA_TYPE_VIDEO, 0);
    if (!ctx)
    {
        return 0;
    }

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = p->year;
    tm.tm_mon = p->month;
    Amy_media_query_everyday_by_month(ctx, &tm, 0, &(p->day));
    Amy_media_close(ctx);
    AMY_LOG_INFO("tuya playback get days:%#x\n", p->day);
    return 0;
}
static int media_stream_playback_query_day_ts_cb(void *arg)
{
    C2C_TRANS_QUERY_PB_DAY_RESP *p = (C2C_TRANS_QUERY_PB_DAY_RESP *)arg;
    if (!p)
    {
        return 0;
    }
    TIME_T time_utc;
    int time_zone;
    if (tuya_ipc_get_service_time_force(&time_utc, &time_zone))
    {
        AMY_LOG_ERROR("tuya get server time failed\n");
        return -1;
    }

    AMY_LOG_INFO("tuya playback query (%d-%d-%d)\n", p->year, p->month, p->day);
    Amy_media_context *ctx = Amy_media_open(TFCARD_MOUNT_PATH, MEDIA_TYPE_VIDEO, 1000);
    if (!ctx)
    {
        return 0;
    }

    int total = 0;
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = p->year;
    tm.tm_mon = p->month;
    tm.tm_mday = p->day;
    media_file_info_t *pinfo = Amy_media_query_by_day(ctx, &tm, 0, &total);
    if ((!pinfo) && (total <= 0))
    {
        Amy_media_close(ctx);
        AMY_LOG_ERROR("query day failed(%d-%d-%d)\n", tm.tm_year, tm.tm_mon, tm.tm_mday);
        return 0;
    }

    PLAY_BACK_ALARM_INFO_ARR *alarm_arr = (PLAY_BACK_ALARM_INFO_ARR *)malloc(sizeof(PLAY_BACK_ALARM_INFO_ARR) + total * sizeof(PLAY_BACK_ALARM_FRAGMENT));
    if (!alarm_arr)
    {
        p->alarm_arr = NULL;
        Amy_media_close(ctx);
        return 0;
    }
    alarm_arr->file_count = total;
    for (int i = 0; i < total; i++)
    {
        alarm_arr->file_arr[i].type = 0;
        alarm_arr->file_arr[i].time_sect.start_timestamp = (unsigned int)strtoul(pinfo[i].timestamp, NULL, 10) - time_zone;
        alarm_arr->file_arr[i].time_sect.end_timestamp = (unsigned int)(alarm_arr->file_arr[i].time_sect.start_timestamp + strtoul(pinfo[i].duration, NULL, 10));
    }
    p->alarm_arr = alarm_arr;
    AMY_LOG_INFO("tuya playback get days:%#x\n", p->day);
    return 0;
}

static int media_stream_playback_query_day_ts_encrypt_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}

static void *tuya_pb_task(void *arg)
{
    pb_context *ctx = (pb_context *)arg;

    ctx->audio_buf = NULL;
    ctx->audio_buf_size = AVI_audio_channels(ctx->avi_dev) *
                          AVI_audio_rate(ctx->avi_dev) *
                          AVI_audio_bits(ctx->avi_dev) * ctx->oneframe_duration / (8 * 1000);

    if (ctx->audio_buf_size > 0)
    {
        ctx->audio_buf = (unsigned char *)malloc(ctx->audio_buf_size);
    }
    while (ctx->run)
    {
        if (ctx->pause)
        {
            Amy_msleep(100);
        }
        if (ctx->audio_buf)
        {
            if (AVI_set_video_position(ctx->avi_dev, ctx->video_frame_index))
            {
                AMY_LOG_ERROR("video set position failed\n");
                Amy_msleep(10);
                continue;
            }

            long size = AVI_frame_size(ctx->avi_dev, ctx->video_frame_index);
            unsigned char *buf = (unsigned char *)malloc(size);
            if (buf)
            {
                int keyframe = 0;
                if (AVI_read_frame(ctx->avi_dev, (char *)buf, &keyframe) >= 0)
                {
                    MEDIA_VIDEO_FRAME_T video_frame = {0};
                    video_frame.video_codec = TUYA_CODEC_VIDEO_H264;
                    video_frame.video_frame_type = keyframe ? E_VIDEO_I_FRAME : E_VIDEO_PB_FRAME;
                    video_frame.width = AVI_video_width(ctx->avi_dev);
                    video_frame.height = AVI_video_height(ctx->avi_dev);
                    video_frame.fps = AVI_frame_rate(ctx->avi_dev);
                    video_frame.p_video_buf = buf;
                    video_frame.buf_len = size;
                    video_frame.timestamp = (unsigned long long)(ctx->start_timestamp) * 1000 + ctx->oneframe_duration * ctx->video_frame_index;
                    struct tm tm;
                    time_t sec = video_frame.timestamp / 1000;
                    Amy_time2localtime(sec, tm);
                    video_frame.pts = video_frame.timestamp * 1000;
                    printf("================send playback data to app\r\n");
                    tuya_ipc_media_playback_send_video_frame(ctx->ch, &video_frame);

                    //  AVI_set_audio_position(ctx->avi_dev, ctx->video_frame_index * ctx->audio_buf_size);
                    if (AVI_read_audio(ctx->avi_dev, (char *)ctx->audio_buf, ctx->audio_buf_size) > 0)
                    {
                        MEDIA_AUDIO_FRAME_T audio_frame = {0};
                        audio_frame.audio_codec = TUYA_CODEC_AUDIO_PCM;
                        audio_frame.audio_sample = AVI_audio_rate(ctx->avi_dev);
                        ;
                        audio_frame.audio_databits = AVI_audio_bits(ctx->avi_dev);
                        audio_frame.audio_channel = AVI_audio_channels(ctx->avi_dev);
                        audio_frame.p_audio_buf = ctx->audio_buf;
                        audio_frame.buf_len = ctx->audio_buf_size;
                        audio_frame.timestamp = video_frame.timestamp;
                        audio_frame.pts = audio_frame.timestamp * 1000;
                        tuya_ipc_media_playback_send_audio_frame(ctx->ch, &audio_frame);
                    }
                    ctx->video_frame_index++;
                }
                free(buf);
            }
        }
        Amy_msleep(30);
    }
    if (ctx->audio_buf)
    {
        free(ctx->audio_buf);
    }
    return NULL;
}

static int media_stream_playback_video_start_ts_cb(void *arg)
{
    C2C_TRANS_CTRL_PB_START *p = (C2C_TRANS_CTRL_PB_START *)arg;
    char path[128] = {0};
    struct tm tm;
    TIME_T time_utc;
    int time_zone;
    if (p->channel > PB_CLIENT_MAX)
    {
        AMY_LOG_ERROR("playback channels :%d\n", p->channel);
        return -1;
    }
    if ((pb_ctx[p->channel].avi_dev) &&
        ((pb_ctx[p->channel].start_timestamp != p->time_sect.start_timestamp) || (pb_ctx[p->channel].pause)))
    {
        if (pb_ctx[p->channel].run)
        {
            pb_ctx[p->channel].run = 0;
            Amy_pthread_join(pb_ctx[p->channel].tid, NULL);
        }
        if (pb_ctx[p->channel].avi_dev)
        {
            AVI_close(pb_ctx[p->channel].avi_dev);
            pb_ctx[p->channel].avi_dev = NULL;
        }
    }
    pb_ctx[p->channel].ch = p->channel;
    pb_ctx[p->channel].id = p->reqId;
    pb_ctx[p->channel].start_timestamp = p->time_sect.start_timestamp;
    pb_ctx[p->channel].current_timestamp = p->playTime;
    pb_ctx[p->channel].duration = (p->time_sect.end_timestamp - p->time_sect.start_timestamp);
    pb_ctx[p->channel].pause = 0;
    if (!pb_ctx[p->channel].avi_dev)
    {
        if (tuya_ipc_get_service_time_force(&time_utc, &time_zone))
        {
            AMY_LOG_ERROR("tuya get server time failed\n");
            return -1;
        }
        pb_ctx[p->channel].time_zone = time_zone;
        unsigned int timestmap = p->time_sect.start_timestamp + time_zone;
        Amy_time2localtime(timestmap, tm);
        snprintf(path, sizeof(path) - 1, "%s/%d/%d/%d/%d_%d.avi", TFCARD_MEDIA_PATH, tm.tm_year, tm.tm_mon, tm.tm_mday, timestmap, pb_ctx[p->channel].duration);
        AMY_LOG_INFO("play back open :%s\n", path);

        pb_ctx[p->channel].avi_dev = AVI_open_input_file(path, 1);
        if (!pb_ctx[p->channel].avi_dev)
        {
            AMY_LOG_ERROR("open %s fialed\n", path);
            return -1;
        }

        pb_ctx[p->channel].run = 1;
        pb_ctx[p->channel].video_frame_index = 0;
        pb_ctx[p->channel].oneframe_duration = 1000 / AVI_frame_rate(pb_ctx[p->channel].avi_dev);
        Amy_pthread_create(&pb_ctx[p->channel].tid, tuya_pb_task, &pb_ctx[p->channel], 32 * 1024, 12);
    }
    else
    {
        int total_frame = AVI_video_frames(pb_ctx[p->channel].avi_dev);
        int frame_index = (p->playTime - p->time_sect.start_timestamp) * AVI_frame_rate(pb_ctx[p->channel].avi_dev);
        pb_ctx[p->channel].video_frame_index = frame_index < 0 ? 0 : frame_index > total_frame ? total_frame
                                                                                               : frame_index;
    }
    return 0;
}
static int media_stream_playback_pause_cb(void *arg)
{
    C2C_TRANS_CTRL_PB_PAUSE *p = (C2C_TRANS_CTRL_PB_PAUSE *)arg;
    pb_ctx[p->channel].pause = 1;
    return 0;
}
static int media_stream_playback_resume_cb(void *arg)
{
    C2C_TRANS_CTRL_PB_RESUME *p = (C2C_TRANS_CTRL_PB_RESUME *)arg;
    pb_ctx[p->channel].pause = 0;
    return 0;
}
static int media_stream_playback_mute_cb(void *arg)
{
    return 0;
}
static int media_stream_playback_unmute_cb(void *arg)
{
    return 0;
}
static int media_stream_playback_set_speed_cb(void *arg)
{
    return 0;
}
static int media_stream_playback_stop_cb(void *arg)
{
    C2C_TRANS_CTRL_PB_STOP *p = (C2C_TRANS_CTRL_PB_STOP *)arg;
    if (pb_ctx[p->channel].run)
    {
        pb_ctx[p->channel].run = 0;
        Amy_pthread_join(pb_ctx[p->channel].tid, NULL);
    }
    if (pb_ctx[p->channel].avi_dev)
    {
        AVI_close(pb_ctx[p->channel].avi_dev);
        pb_ctx[p->channel].avi_dev = NULL;
    }
    return 0;
}
static int media_stream_live_video_clarity_set_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_live_video_clarity_query_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_download_start_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_download_start_width_encrypt_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_download_stop_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_download_pause_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_download_cancle_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_album_query_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_album_download_start_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_album_download_cancel_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_album_delete_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static int media_stream_album_play_ctrl_cb(void *arg)
{
    AMY_LOG_ERROR("<<<-----------------\n");
    return 0;
}
static media_event_cb_t media_event_cb_maps[] = {
    {MEDIA_STREAM_LIVE_VIDEO_START, media_stream_video_start_cb},
    {MEDIA_STREAM_LIVE_VIDEO_STOP, media_stream_video_stop_cb},
    {MEDIA_STREAM_LIVE_AUDIO_START, media_stream_audio_start_cb},
    {MEDIA_STREAM_LIVE_AUDIO_STOP, media_stream_audio_stop_cb},
    {MEDIA_STREAM_SPEAKER_START, media_stream_speaker_start_cb},
    {MEDIA_STREAM_SPEAKER_STOP, media_stream_speaker_stop_cb},
    {MEDIA_STREAM_DISPLAY_START, media_stream_display_start_cb},
    {MEDIA_STREAM_DISPLAY_STOP, media_stream_display_stop_cb},
    {MEDIA_STREAM_LIVE_VIDEO_SEND_PAUSE, media_stream_video_send_pause_cb},
    {MEDIA_STREAM_LIVE_VIDEO_SEND_RESUME, media_stream_video_send_resume_cb},
    {MEDIA_STREAM_LIVE_LOAD_ADJUST, media_stream_load_adjust_cb},
    {MEDIA_STREAM_PLAYBACK_LOAD_ADJUST, media_stream_playback_load_adust_cb},
    {MEDIA_STREAM_ABILITY_QUERY, media_stream_ability_query_cb},
    {MEDIA_STREAM_PLAYBACK_QUERY_MONTH_SIMPLIFY, media_stream_playback_query_month_simplify_cb},
    {MEDIA_STREAM_PLAYBACK_QUERY_DAY_TS, media_stream_playback_query_day_ts_cb},
    {MEDIA_STREAM_PLAYBACK_QUERY_DAY_TS_WITH_ENCRYPT, media_stream_playback_query_day_ts_encrypt_cb},
    {MEDIA_STREAM_PLAYBACK_START_TS, media_stream_playback_video_start_ts_cb},
    {MEDIA_STREAM_PLAYBACK_PAUSE, media_stream_playback_pause_cb},
    {MEDIA_STREAM_PLAYBACK_RESUME, media_stream_playback_resume_cb},
    {MEDIA_STREAM_PLAYBACK_MUTE, media_stream_playback_mute_cb},
    {MEDIA_STREAM_PLAYBACK_UNMUTE, media_stream_playback_unmute_cb},
    {MEDIA_STREAM_PLAYBACK_SET_SPEED, media_stream_playback_set_speed_cb},
    {MEDIA_STREAM_PLAYBACK_STOP, media_stream_playback_stop_cb},
    {MEDIA_STREAM_LIVE_VIDEO_CLARITY_SET, media_stream_live_video_clarity_set_cb},
    {MEDIA_STREAM_LIVE_VIDEO_CLARITY_QUERY, media_stream_live_video_clarity_query_cb},
    {MEDIA_STREAM_DOWNLOAD_START, media_stream_download_start_cb},
    {MEDIA_STREAM_DOWNLOAD_START_WITH_ENCRYPT, media_stream_download_start_width_encrypt_cb},
    {MEDIA_STREAM_DOWNLOAD_STOP, media_stream_download_stop_cb},
    {MEDIA_STREAM_DOWNLOAD_PAUSE, media_stream_download_pause_cb},
    {MEDIA_STREAM_DOWNLOAD_CANCLE, media_stream_download_cancle_cb},
    {MEDIA_STREAM_ALBUM_QUERY, media_stream_album_query_cb},
    {MEDIA_STREAM_ALBUM_DOWNLOAD_START, media_stream_album_download_start_cb},
    {MEDIA_STREAM_ALBUM_DOWNLOAD_CANCEL, media_stream_album_download_cancel_cb},
    {MEDIA_STREAM_ALBUM_DELETE, media_stream_album_delete_cb},
    {MEDIA_STREAM_ALBUM_PLAY_CTRL, media_stream_album_play_ctrl_cb},
};

static int media_event_cb(const int devcie, int chn, const MEDIA_STREAM_EVENT_E ev, void *arg)
{
    int cnt = sizeof(media_event_cb_maps) / sizeof(media_event_cb_t);
    for (int i = 0; i < cnt; i++)
    {
        if (ev == media_event_cb_maps[i].ev)
        {
            if (media_event_cb_maps[i].cb)
            {
                return media_event_cb_maps[i].cb(arg);
            }
        }
    }
    AMY_LOG_WARNING("media event not support cmd:%d\n", ev);
    return 0;
}

int tuya_media_stream_init(void)
{
    MEDIA_STREAM_VAR_T var;
    memset(&var, 0, sizeof(MEDIA_STREAM_VAR_T));
    var.on_event_cb = media_event_cb;
    var.max_client_num = 5;
    var.recv_buffer_size = 16 * 1024;
    int ret = tuya_ipc_media_stream_init(&var);
    if (ret)
    {
        AMY_LOG_ERROR("media stream init failed\n");
    }
    return ret;
}

int tuya_media_stream_pause(void)
{
    if (tuya_mqtt_is_online() == false)
    {
        return 0;
    }
    return tuya_ipc_media_service_pause();
}