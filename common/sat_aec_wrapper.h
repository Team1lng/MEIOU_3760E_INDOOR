#ifndef SAT_AEC_WRAPPER_H
#define SAT_AEC_WRAPPER_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief 创建 AEC 实例
   * @param sample_rate 采样率（8000/16000/32000）
   * @return AEC 句柄，NULL 表示失败
   */
  void *sat_aec_create(int sample_rate);

  /**
   * @brief 销毁 AEC 实例
   * @param aec sat_aec_create 返回的句柄
   */
  void sat_aec_destroy(void *aec);

  /**
   * @brief 获取全局 AEC 句柄（渲染路径使用）
   * @return AEC 句柄，NULL 表示未初始化
   */
  void *sat_aec_global_handle(void);

  /**
   * @brief 设置全局 AEC 句柄
   * @param aec sat_aec_create 返回的句柄
   */
  void sat_aec_set_global_handle(void *aec);

  /**
   * @brief 喂入渲染参考信号（喇叭即将播放的音频）
   * @param aec    AEC 句柄
   * @param pcm    16-bit 单声道 PCM 数据
   * @param samples 采样点数
   */
  void sat_aec_render(void *aec, const int16_t *pcm, int samples);

  /**
   * @brief 处理采集信号（消除回声，原地修改）
   * @param aec    AEC 句柄
   * @param pcm    16-bit 单声道 PCM 数据（原地处理）
   * @param samples 采样点数
   */
  void sat_aec_capture(void *aec, int16_t *pcm, int samples);

#ifdef __cplusplus
}
#endif

#endif // SAT_AEC_WRAPPER_H
