/* 
 *  @file product_test_usb.h
 *  @brief product_test_usb module is supply usb test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_USB_H__
#define __PRODUCT_TEST_USB_H__

#define MAX_USB_DEVICE (16)

typedef struct usb_inode_s
{
    int vendor_id;
    int device_id;
}USB_INODE_T;

typedef struct usb_info_s
{
    int n;
    USB_INODE_T usb[MAX_USB_DEVICE];
}USB_INFO_T;

/*@brief get usb info
 *@param(out) info: get usb info,see also USB_INFO_T
 *@param(out)value: adc value
 *@return 0:succeed,-1:fail
 */
extern int product_usb_get_usb_info(USB_INFO_T *info);

#endif
