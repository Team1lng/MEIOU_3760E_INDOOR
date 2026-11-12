/* 
 *  @file product_test_flash.h
 *  @brief product_test_flash module is supply flash test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_FLASH_H__
#define __PRODUCT_TEST_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief flash test 
 *@param(in) addr:flash test address
 *@param(in) size:flash test size
 *@param(in)times:flash test times 
 *@return 0:succeed,-1:fail
 */
extern int product_test_flash_test(int addr , int size,int times);

/*@brief flash get size 
 *@param(out) size:flash total size 
 *@return 0:succeed,-1:fail
 */
extern int product_test_flash_get_size(uint32_t *size);


/*@brief get flash total size
 *@param(out) read:read spead KBit/s 
 *@param(out) write:write spead KBit/s 
 *@return 0:succeed,-1:fail
 */
extern int product_test_flash_get_speed(uint32_t *read,uint32_t* write); 

#ifdef __cplusplus
}
#endif

#endif
