/*
 * ADC space access commands.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <adc.h>

/*
 * adc Display
 *
 * Syntax:
 *	iod{.r} {addr}
 */
int do_read_adc(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if(adc_setting(7) == 0) 
        adc1_read_ain0();

	return 0;
}

/**************************************************/
U_BOOT_CMD(read_adc, 1, 0 , do_read_adc,
	   "read adc value", "read_adc -times");

