#include <hal_event.h>
#include <stdio.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <fcntl.h>
#include <stdlib.h>

void print_uevent_msg(char *msg){

        int i = 0;
        while(i < UEVENT_MSG_LEN+2){
            if(!msg[i]){
                printf("!");
                i++;
                continue;
            }
            putchar(msg[i]);
            i++;
        }
        printf("\n");

}

int main(int argc, char* argv[]){

    hw_event_listener listener;

    int ret = 0;
    hw_event* event = malloc(sizeof(hw_event));
    memset(event, 0, sizeof(hw_event));

    hw_battery_event battery_result;
    hw_usb_power_event usb_power_result;
    hw_usb_storage_event usb_storage_result;
    hw_sdcard_event sdcard_result;

    /* eth event variable init */
    hw_eth_event_list eth_event_result_list;
    eth_event_result_list.size = 50;
    eth_event_result_list.count = 0;
    eth_event_result_list.head = malloc(sizeof(hw_eth_event) * eth_event_result_list.size);
    memset(eth_event_result_list.head, 0, sizeof(hw_eth_event) * eth_event_result_list.size);
    hw_eth_event* eth_event_result;

    printf("hw_event_test V1.0\n");

    /* register hw event */
    ret = hal_hw_event_register(&listener, ETH_EVENT | BATTERY_EVENT | USB_POWER_EVENT | SDCARD_EVENT | USB_STORAGE_EVENT);
    if (ret < 0) {
        printf("%s: hw_event_register failed ret:%d\n", __func__, ret);
        free(event);
        free(eth_event_result_list.head);
        return ret;
    }

    printf("RTM_NEWADDR:%d RTM_DELADDR:%d RTM_GETADDR:%d NLMSG_ERROR:%d\n", RTM_NEWADDR, RTM_DELADDR, RTM_GETADDR, NLMSG_ERROR);
    printf("RTM_NEWLINK:%d RTM_DELLINK:%d RTM_GETLINK:%d NLMSG_DONE:%d\n", RTM_NEWLINK, RTM_DELLINK, RTM_GETLINK, NLMSG_DONE);

    /* recv hw event */
    int i = 0;
    for(i = 0; i < 60; i++)
    {
        ret = hal_hw_event_recv(&listener, event, 0);
        printf("after hal_hw_event_recv ret:%d msg_len:%d\n", ret, event->msg_len);
        if (ret < 0) {
            printf("%s: hw_event_recv failed ret:%d\n", __func__, ret);
            free(event);
            free(eth_event_result_list.head);
            return ret;
        }

        /* parse battery event */
        if(IS_BATTERY_EVENT(event->type)){
            ret = hal_hw_parse_battery_event(event, &battery_result);
            if(ret < 0){
                printf("hw_parse_event failed ret:%d\n", ret);
                continue;
            }
            printf("%s: battery_status:%s battery_present:%d battery_capacity:%d battery_health:%s\n",
            __func__, battery_result.battery_status, battery_result.battery_present, battery_result.battery_capacity, battery_result.battery_health);
        }

        /* parse usb_power event */
        if (IS_USB_POWER_EVENT(event->type)){
            ret = hal_hw_parse_usb_power_event(event, &usb_power_result);
            if(ret < 0){
                printf("hw_parse_event failed ret:%d\n", ret);
                continue;
            }
            printf("%s: usb_power_online:%d\n", __func__, usb_power_result.power_online);
        }

        /* parse usb_storage event */
        if (IS_USB_STORAGE_EVENT(event->type)){
            ret = hal_hw_parse_usb_storage_event(event, &usb_storage_result);
            if(ret < 0){
                printf("hw_parse_event failed ret:%d\n", ret);
                continue;
            }
            printf("%s: usb_storage_plug:%d\n", __func__, usb_storage_result.usb_storage_plug);
        }

        /* parse sd card event */
        if (IS_SDCARD_EVENT(event->type)){
            ret = hal_hw_parse_sdcard_event(event, &sdcard_result);
            if(ret < 0){
                printf("hw_parse_event failed ret:%d\n", ret);
                continue;
            }
            printf("%s: sd_insert:%d\n", __func__, sdcard_result.sd_plug);
        }

        /* parse eth event */
        if (IS_ETH_EVENT(event->type)){
            ret = hal_hw_parse_eth_event(event, &eth_event_result_list);
            if(ret < 0){
                printf("hw_parse_event failed ret:%d\n", ret);
                continue;
            }
            eth_event_result = eth_event_result_list.head;
            int j = 0;
            for(j = 0; j < eth_event_result_list.count; j++){
                printf("%s: eth_name:%s type:%d state:%d\n", __func__, eth_event_result->device_name, eth_event_result->type, eth_event_result->device_state);
                eth_event_result++;
            }
        }

        memset(event, 0, sizeof(hw_event));
        memset(eth_event_result_list.head, 0, sizeof(hw_eth_event) * eth_event_result_list.size);
    }

    /* unregister hw event */
    ret = hal_hw_event_unregister(&listener);
    if (ret < 0) {
        printf("%s: hw_event_unregister failed ret:%d\n", __func__, ret);
        free(event);
        free(eth_event_result_list.head);
        return ret;
    }

    free(event);
    free(eth_event_result_list.head);
    return 0;
}



