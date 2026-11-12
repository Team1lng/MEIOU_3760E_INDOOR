
#ifndef _ADC_H_
#define _ADC_H_

//adc.h

#define CONFIG_ADC
#define CONFIG_CMD_ADC

#define AK_VA_SYSCTRL			0x08000000
//#define AK_PA_SYSCTRL			(0x08000000)
//#define AK_SZ_SYSCTRL			SZ_32K

#define AD_RESET_CTRL_REG       (AK_VA_SYSCTRL + 0x00000020)
#define AD_DA_CLK1_REG			(AK_VA_SYSCTRL + 0x0000000C)
#define SAR_ADC_CFG_REG			(AK_VA_SYSCTRL + 0x00000098)
#define SAR_ADC_ACR_REG1		(AK_VA_SYSCTRL + 0x0000009C)
#define SAR_ADC_ACR_REG2		(AK_VA_SYSCTRL + 0x000000A0)    
#define SAR_ADC_ACR_REG3		(AK_VA_SYSCTRL + 0x000000A4)
#define SAR_IF_CFG_REG			(AK_VA_SYSCTRL + 0x0000005C)
#define SAR_TIMING_CFG_REG		(AK_VA_SYSCTRL + 0x00000060)
#define SAR_THRESHOLD_REG		(AK_VA_SYSCTRL + 0x00000064)
#define SAR_IF_SMP_DAT_REG		(AK_VA_SYSCTRL + 0x00000068)
#define SAR_IF_INT_STATUS_REG	(AK_VA_SYSCTRL + 0x0000006C)
#define BAT_AND_TS_THREHOLD_REG	(AK_VA_SYSCTRL + 0x000000EC)  /* BAT AND TS THRESHOLD*/
#define BAT_SAMPLE_VAL_REG		(AK_VA_SYSCTRL + 0x000000F0)  /* BAT Sample DATA */
#define XP_XN_SAMPLE_VAL_REG	(AK_VA_SYSCTRL + 0x000000F4)  /* XP & XN SAMPLE VAL*/
#define YP_YN_SAMPLE_VAL_REG	(AK_VA_SYSCTRL + 0x000000F8)

#define REG32_CONTROL(_register_)                               (*(volatile unsigned long *)(_register_))

#define ADC1_MAIN_CLK			(24000000)

#define ADC1_DEFAULT_CLK		(4000000)
#define DEFAULT_SAMPLE			(5000)
#define AK_AVCC				(3300)			/* AVCC always 3.3V */

#define AK_ADC1_AD0			(0)
#define AK_ADC1_AD1			(1)
#define AK_ADC1_BAT			(2)
#define AK_ADC1_XP			(3)
#define AK_ADC1_XN			(4)
#define AK_ADC1_YP			(5)
#define AK_ADC1_YN			(6)

#define adc1_read_bat()			adc1_read_channel(AK_ADC1_BAT)
#define adc1_read_ad5()			adc1_read_channel(AK_ADC1_AD1)
#define adc1_read_ain0()		adc1_read_channel(AK_ADC1_AD0)
#define adc1_read_ain1()		adc1_read_channel(AK_ADC1_AD1)

#define read_adc_value(times)        do{ if(adc_setting(times) == 0) adc1_read_ain0(); }while(0)

#define SARADC_TIMEOUT			msecs_to_jiffies(100)

typedef enum SARADC_VERF_SEL_TYPE
{
	SARADC_VREF_USE_HPVDD = 0,
	SARADC_VREF_USE_BGR,

}T_SARADC_VERF_SEL_TYPE;

unsigned long adc1_read_channel(int channel);
int adc_setting(unsigned int set_sampling_num);

#endif

