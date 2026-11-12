add_definitions(
  -DHAVE_CONFIG_H
  -DHAVE_STDINT_H
)
include_directories(
  ./TuyaOS/include/components/svc_api_ipc/include/
  ./TuyaOS/include/components/svc_upgrade/include/
  ./TuyaOS/include/components/svc_wifi/include/
  ./TuyaOS/include/components/svc_devos/include/
  ./TuyaOS/include/components/lib_cJSON/include/
  ./TuyaOS/include/components/svc_cloud_storage/include/
  ./TuyaOS/include/components/tal_security/include/
  ./TuyaOS/include/components/base_media/include/
  ./TuyaOS/include/components/svc_ipc_core/include/
  ./TuyaOS/include/components/svc_video_msg/include/
  ./TuyaOS/include/components/svc_ring_buffer/include/
  ./TuyaOS/include/components/svc_streaming_p2p/include/
  ./TuyaOS/include/components/svc_ipc_tmm_control/include/
  ./TuyaOS/include/components/svc_local_storage/include/
  ./TuyaOS/include/components/svc_rule/include/
  ./TuyaOS/include/components/svc_netmgr/include//

  ./TuyaOS/include/components/tal_system/include/
  ./TuyaOS/include/components/tal_system_service/include/
    ./TuyaOS/include/components/tal_system_service/include/utilities/
  ./TuyaOS/include/components/svc_net_storage/include/
  ./TuyaOS/include/components/svc_tuya_cloud/include/iot_dns/
  ./TuyaOS/include/components/tal_network/include/
    ./TuyaOS/include/components/svc_api_iot/include/
      ./TuyaOS/include/components/tal_wifi/include/
  ./TuyaOS/include/base/include/

  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/utilities/include/
  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/security/
  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/storage/
  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/init/include/
  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/system/
  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/wifi/
  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/wired/
  ./TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/media/


  ./TuyaOS/vendor/ak3760e/tuyaos/components/bluetooth/include/
  ./TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/log/
  ./TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/include/
  ./TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/
  ./TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/iwlib/
  ./TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/func/wpa_s/


  ./TuyaOS/apps/tuyaos_demo_ipc/application_components/app_main/include/

  # 项目层头文件（monitor.h 等）
  ${CMAKE_CURRENT_SOURCE_DIR}/../../project/
  )


set(src
  tuya_sdk.c
  tuya_status_change.c
  tuya_media_event.c
  tuya_stream_storage.c
  tuya_call.c
  tuya_uuid_and_key.c
  tuya_audio_recv.c
  tuya_dp_handle.c
  tuya_dp_callback.c
  tuya_sdk_upgrade.c

  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/utilities/src/tuya_ringbuf.c
  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/utilities/src/tuya_list.c
  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/utilities/src/tuya_tools.c
  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/init/src/tkl_init_system.c
  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/init/src/tkl_init_wifi.c
  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/init/src/tkl_init_wired.c
  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/utilities/src/tuya_smartpointer.c
  TuyaOS/vendor/ak3760e/tuyaos/tuyaos_adapter/include/utilities/src/tuya_queue.c

  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_bluetooth/src/tkl_bluetooth.c

  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_fs.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_queue.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_semaphore.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_memory.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_mutex.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_thread.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_ota.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_output.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_system.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_system/src/tkl_sleep.c

  
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/func/tkl_gen_net.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/func/tkl_net.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/sys_cmd.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/func/tkl_wifi.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/pcap.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/bpf_filter.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/pcap-usb-linux.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/optimize.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/gencode.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/nametoaddr.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/grammar.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/scanner.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/inet.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/savefile.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/sf-pcap-ng.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/pcap-common.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/sf-pcap.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/pcap-linux.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/libpcap-1.4.0/fad-getad.c

  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wifi/src/utils/iwlib/iwlib.c

  
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wired/src/tkl_wired.c
  TuyaOS/vendor/ak3760e/tuyaos/components/tkl_wired/src/tkl_wired.c
  
  TuyaOS/apps/tuyaos_demo_ipc/application_components/app_main/src/ty_sdk_sd_card.c

  )
  add_compile_options(
  -Wno-unused-value
)

add_library(tuya OBJECT
  ${src}
)

