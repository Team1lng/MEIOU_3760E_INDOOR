#ifndef __APP_INTERCOM_H__
#define __APP_INTERCOM_H__

int intercom_audio_open(void);

int intercom_audio_close(void);

int intercom_device_info_set(const device_info_attr *info);

int intercom_get_current_device(device_info_attr *out_info);

int intercom_volume_set(int volume);

int intercom_volume_get(void);

#endif //__APP_INTERCOM_H__