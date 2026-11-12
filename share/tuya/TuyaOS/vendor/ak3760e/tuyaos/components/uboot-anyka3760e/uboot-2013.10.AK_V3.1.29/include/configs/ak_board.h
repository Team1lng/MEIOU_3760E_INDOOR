#ifndef __AK_BOARD__H
#define __AK_BOARD__H

/*#define CONFIG_MIPI_CORE_BOARD*/
#define CONFIG_RGB_CORE_BOARD

#define CONFIG_DRAM_128M

/*for spi bus width 4 wires */ 
#define CONFIG_SPI_FLASH_BUSWIDTH_4X

/*for RT-thread DTB addr*/ 
/*#define CONFIG_RTT_DTB_ADDR*/


/*need two boot config*/
#define CONFIG_SPL_BOOT

/*
the flag is for SPI NOR for SPINAND,
open the flag is SPINAND, close is SPINOR
*/

#define CONFIG_SPINAND_FLASH

/*
* Enable the double mac
*/
/* #define AK_DOUBLE_ETH_SUPPORT */

#endif/* __AK_BOARD__H */
