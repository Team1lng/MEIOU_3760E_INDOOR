#include <config.h>
#include <common.h>
#include <command.h>
#include <part.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch-ak37e/ak_cpu.h>
#include <adc.h>

static unsigned int vref_select;
static unsigned int battery_divider;
static unsigned int sampling_num;

/* div     SARADC CLK = 24Mhz/(div+1); */
static void adc1_clk_cfg(unsigned long div)
{
	unsigned long saradc_chief_driven;
	unsigned long saradc_module_driven;
    unsigned long regval = 0;

	/*
	*            reserve the driven value
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
	saradc_chief_driven = ((__raw_readl(SAR_IF_CFG_REG)) & (0x1<<0));
    saradc_module_driven = ((__raw_readl(SAR_IF_CFG_REG)) & (0xf<<5));

	/* disable module chief driven by sar adc clk  */
    REG32_CONTROL(SAR_IF_CFG_REG) &= ~(1<<0);

	/*
	*            disable Ain0_sampling   Ain1_sampling   Bat_sampling
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(0xf<<5));

	/* disable sar adc clk */
    REG32_CONTROL(AD_DA_CLK1_REG) &= (~(0x1<<3));

	/* cofig div */
    REG32_CONTROL(AD_DA_CLK1_REG) &= (~0x7);

    REG32_CONTROL(AD_DA_CLK1_REG) |= (div & 0x7);

	/* open sar adc clk */
    REG32_CONTROL(AD_DA_CLK1_REG) |= (0x1<<3);

	/* get back the driven cfg */
    REG32_CONTROL(SAR_IF_CFG_REG) |= (saradc_chief_driven | saradc_module_driven);
}

static void power_on_adc1(void)
{
    unsigned long regval = 0;
	unsigned long test;

    REG32_CONTROL(SAR_ADC_ACR_REG2) &= (~(1 << 0));

//	test = (REG32_CONTROL(SAR_ADC_ACR_REG2) &  (1<<0));
//	printf("power on or not : %ld \n" , test);
}

static void power_off_adc1(void)
{
    unsigned long regval = 0;
	unsigned long test;

    REG32_CONTROL(SAR_ADC_ACR_REG2) |= (1 << 0);

//	test = (REG32_CONTROL(SAR_ADC_ACR_REG2) &  (1<<0));
//	printf("power on or not : %ld \n" , test);
}

/* channel_type can be ain0, ain1, bat */
static unsigned long adc_get_sample_val(int channel_type)
{
	unsigned long val = 0;
    unsigned long regval = 0;
	int timeout = 2000;

	/* disable  sample  */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(1<<0));

	/* clear sample done interrupt  */
    REG32_CONTROL(SAR_IF_INT_STATUS_REG) &= (~0x1);

	/* enable  sample   */
    REG32_CONTROL(SAR_IF_CFG_REG) |= (1<<0);
	/* wait sample done  */
	while( (REG32_CONTROL(SAR_IF_INT_STATUS_REG) & 1) == 0 && timeout--)
	{
		mdelay(1);
		printf("sample NOT done.\n");
	}
	if(timeout == -1) {
		printf("error : ch%d sample timeout!\n", channel_type);
	}
	/* disable  sample  */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(1<<0));

	if( (channel_type == AK_ADC1_AD0) || (channel_type == AK_ADC1_AD1) ) {
		val = ((REG32_CONTROL(SAR_IF_SMP_DAT_REG)>>(channel_type*12)) & 0xfff);
	}
	else if(channel_type == AK_ADC1_BAT) {
        val = ((REG32_CONTROL(SAR_IF_INT_STATUS_REG)>>20) & 0xffff);
		//val = ((REG32_CONTROL(SAR_IF_INT_STATUS_REG)>>20) & 0xfff);
	}

	return val;
}

/* channel_type: SARADC_AIN0, SARADC_AIN1, SARADC_BAT */
static void adc_close_channel(int channel_type)
{
	unsigned long state;
    unsigned long regval = 0;

	/*
	*          close channel
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
   #if 0
    regval = (1<<(channel_type+5));
	if( (REG32_CONTROL(SAR_IF_CFG_REG) & regval) == 0)
	{
		printf("this channel already close.\n");
	}
	#endif

	/* reserve the sample switch state */
	state = (REG32_CONTROL(SAR_IF_CFG_REG) & (1<<0));

	/* close sample */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(1<<0));

	/* clear channel interrupt state and sample interrupt state */
    REG32_CONTROL(SAR_IF_INT_STATUS_REG) &= (~((1<<0) | (1<<(channel_type+1))));

    /*
    *                  close channel
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(1<<(channel_type+5)));

	if( channel_type == AK_ADC1_BAT)
	{
		/* disable bat div ratio */
        REG32_CONTROL(SAR_ADC_ACR_REG2) &= (~(1<<7));
	}

	/* resume the sample switch state */
    REG32_CONTROL(SAR_IF_CFG_REG) |= state;
}

/* channel_type : SARADC_AIN0, SARADC_AIN1, SARADC_BAT */
static unsigned long adc_config_channel( unsigned long clkdiv,int channel_type)
{
	unsigned long samplerate;
	unsigned long adc1_clk;
	unsigned long spl_cycle, spl_hold, spl_wait,cycle_time;
    unsigned long regval = 0;

	/* make sure the saradc is power on */
	if( (REG32_CONTROL(SAR_ADC_ACR_REG2) &  (1<<0))  ==  (1<<0))
	{
		printf("saradc is power down, please initial first.\n");
	}

	/* disable sample */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(1<<0));
	
	/* clear all interrupt state */
    REG32_CONTROL(SAR_IF_INT_STATUS_REG) = 0x0;
	
	/*
    *                  enable channel
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
    REG32_CONTROL(SAR_IF_CFG_REG)  |= ( 1<<(5+channel_type) );
  
    /* config sample rate */
	/* one channel one time, default samplerate is 5000 */
	adc1_clk = ADC1_MAIN_CLK / (clkdiv + 1);
    /* should be 4 times sample_rate for touch screen . */
    if(channel_type > AK_ADC1_BAT){
	    samplerate = DEFAULT_SAMPLE * 4;
        cycle_time = 29;
    }else{
        samplerate = DEFAULT_SAMPLE;
        cycle_time = 25;
    }

    /*
     * 设计建议:wait=2  hold=wait+22   cycle>=hold+1
	 * 经实验: 单通道可以在wait=2   hold=wait+20   hold=cycle=22的情况下完成采样
	 * 多通道可以在wait=2   hold=wait+20   cycle=29的情况下完成采样
	 * 需要注意的是，多通道采样，cycle要比wait多27至少，如少于27，则采样通道与寄存器的对应会乱
     */
	spl_cycle = adc1_clk / samplerate;
    if((spl_cycle < cycle_time) || (spl_cycle > 65535) )
    {
		printf("Sardac cycle time must higher than %ld  and  lower than 65536!\n",cycle_time);
		return 0;
	}
	spl_wait = 2;
	spl_hold = spl_wait + 22;

    /*
    * H3/H3B spl_wait(0x0800005C  bit14~bit21)
    * H3D spl_wait(0x0800005C  bit16~bit23)
    */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(0xff << 14));

    REG32_CONTROL(SAR_IF_CFG_REG) |= (spl_wait << 14);

    REG32_CONTROL(SAR_TIMING_CFG_REG) = 0;

    REG32_CONTROL(SAR_TIMING_CFG_REG) = spl_cycle | (spl_hold << 16) ;

	/*  config sample cnt
	*   H3/H3B spl_num(0x0800005C  bit8~bit10)
	*   H3D spl_num(0x0800005C  bit24~bit26)
	*/
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(0x7<<8));

    /*sampling number ranges from 1 to 7. configurate by DTS.*/
    REG32_CONTROL(SAR_IF_CFG_REG) |= (sampling_num<<8);

	/* config threshold */
	{
		/* config threshold val */
		if((channel_type==AK_ADC1_AD0)||(channel_type==AK_ADC1_AD1)) {
            REG32_CONTROL(SAR_THRESHOLD_REG) &= (~(0xfff<<(channel_type*12)));
            REG32_CONTROL(SAR_THRESHOLD_REG) |= (100<<(channel_type*12));
        }

        if(channel_type==AK_ADC1_BAT) {
            REG32_CONTROL(SAR_IF_INT_STATUS_REG) &= (~(0xfff<<8));
            REG32_CONTROL(SAR_IF_INT_STATUS_REG) |= (100<<8);
        }

		/* config threshold polarity */
        REG32_CONTROL(SAR_IF_CFG_REG) &= (~(0x1<<(channel_type+11)));
        REG32_CONTROL(SAR_IF_CFG_REG) |= (1<<(channel_type+11));

		/* config threshold interrupt  */
        REG32_CONTROL(SAR_IF_CFG_REG) &= (~(0x1<<(channel_type+2)));
        REG32_CONTROL(SAR_IF_CFG_REG) |= (1<<(channel_type+2));
	}

    /* enable or disable vbat ratio */
	if( channel_type == AK_ADC1_BAT)
	{
		if( battery_divider == 0 ){
            /* disable bat div ratio */
            REG32_CONTROL(SAR_ADC_ACR_REG2) &= (~(1<<7));

		}else if( battery_divider == 1){
			/* enable the bat div radio  use 1:2 ratio */
            REG32_CONTROL(SAR_ADC_ACR_REG2) |= (1<<7);
		}
	}

	/* config sample done interrupt */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(0x1<<1));

    REG32_CONTROL(SAR_IF_CFG_REG) |= (0<<1);
     
	return 0;
}

/*channel_type: SARADC_AIN0, SARADC_AIN1, SARADC_BAT*/
static void adc_open_channel(int channel_type)
{
	unsigned long state;
    unsigned long regval = 0;

	/* make sure the saradc is power on  */
	if( (REG32_CONTROL(SAR_ADC_ACR_REG2) &  (1<<0))  ==  (1<<0)) {
		printf("saradc is power down, please initial first.\n");
	}

	/*
    *                  check whether the channel already open
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
    regval = (1<<(5+channel_type));
	if( (REG32_CONTROL(SAR_IF_CFG_REG) &  regval) == regval )
	{
		printf("this channel already open.\n");
		return ;
	}

	/* reserve the sample switch state  */
	state = (REG32_CONTROL(SAR_IF_CFG_REG) & (1<<0));

	/* close sample  */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(1<<0));

	/* clear channel interrupt state and sample interrupt state  */
    REG32_CONTROL(SAR_IF_INT_STATUS_REG) &= (~((1<<0) | (1<<(channel_type+1))));

    /*
    *                  enable channel
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
    REG32_CONTROL(SAR_IF_CFG_REG) |= ( 1<<(5+channel_type));

//    regval = (1<<(channel_type+5));
//	printf("this channel open or not ?: %ld, %ld.\n",state, (REG32_CONTROL(SAR_IF_CFG_REG) & regval));

	/* resume the sample switch state  */
    REG32_CONTROL(SAR_IF_CFG_REG) |= state;

//    regval = (1<<(channel_type+5));
//	printf("this channel open or not ?: %ld, %ld.\n",state, (REG32_CONTROL(SAR_IF_CFG_REG) & regval));
 
}

static int adc_open_init(int channel_type)
{
	unsigned long  clkdiv;
    unsigned long regval = 0;

	REG32_CONTROL(AD_RESET_CTRL_REG) |= (1<<30);

	/* config adc1 clk */
	clkdiv = ADC1_MAIN_CLK/ADC1_DEFAULT_CLK - 1;
	clkdiv &= 0x7;
	adc1_clk_cfg(clkdiv);

    /* release reset controller */
    REG32_CONTROL(AD_RESET_CTRL_REG) &= (~(1<<30));
  
	/* disable all sample */
    /*
    * H3/H3B chip register(0x0800005C) bit5 and bit6 enable/disable Ain0/Ain1 sample.
    * H3D chip register(0x0800005C) bit6 and bit7 enable/disable Ain0/Ain1 sample.
    */
    /*H3D chip has 4 adc channel for touch screen */
    REG32_CONTROL(SAR_IF_CFG_REG) &= ( ~( (1<<0) | (0x7<<5) ) );

	/* clear all adc1 interrupt state */
    REG32_CONTROL(SAR_IF_INT_STATUS_REG) = 0x0;

	/* mask all adc1 interrupt */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~ (0xf<<1));

	/* power on saradc */
	power_on_adc1();

    /*reset saradc analog */
    REG32_CONTROL(SAR_ADC_ACR_REG2) &= (~(1<<1));

    REG32_CONTROL(SAR_ADC_ACR_REG2) |= (1<<1);

	/*  vref select  */
    if (vref_select == SARADC_VREF_USE_HPVDD){
        /* use HPVDD(3.3v)*/
        REG32_CONTROL(SAR_ADC_ACR_REG2) &= (~(1<<3));
        REG32_CONTROL(SAR_ADC_ACR_REG2) &= (~(1<<14));

    }else if (vref_select == SARADC_VREF_USE_BGR){
        /* use BGR(3.0v)*/
        REG32_CONTROL(SAR_ADC_ACR_REG2) &= (~(1<<3));
        REG32_CONTROL(SAR_ADC_ACR_REG2) |= (1<<14);

    }

	/* disable  sample   */
    REG32_CONTROL(SAR_IF_CFG_REG) &= (~(1<<0));

	adc_close_channel(channel_type);

	adc_config_channel(clkdiv , channel_type);

	adc_open_channel(channel_type);

	return 0;
}

/* bat_setting */
int adc_setting(unsigned int set_sampling_num)
{
	int ret = 0;

	vref_select		= SARADC_VREF_USE_HPVDD;		//选用ain0通道进行电量测试，参考电压选择HPVDD
	battery_divider = 1;							//无用

	sampling_num	= set_sampling_num;
	if ((sampling_num <= 0)||(sampling_num >= 8)) {
		ret = -1;
		printf("Sampling_num is not correct : %d\n" , sampling_num);
	}

	printf("Sampling is settled.\n" );

	return ret;
}

/* Read AD0/AD1/BAT voltage */
unsigned long adc1_read_channel(int channel)
{
	unsigned long val = 0;
    unsigned short vref;

	adc_open_init(channel);

	val = adc_get_sample_val(channel);
	/*  vref select  */
    if (vref_select == SARADC_VREF_USE_HPVDD){
        /* use HPVDD(3.3v)*/
	    vref = 3300;
    }else if (vref_select == SARADC_VREF_USE_BGR){
        /* use BGR(3.0v)*/
        vref = 3000; 
    }
	printf("value is %ld \n",val);
	if (channel == AK_ADC1_BAT)
	{
        if( battery_divider == 0 ){

            /* disable bat div ratio */
            val = val*(vref)/4095;
		    printf(" battery val is %ld mv\n",val);

		}else if( battery_divider == 1){

			/* enable the bat div radio  use 1:2 ratio */
            val = val*(2)*(vref)/4095;
		    printf(" battery val is %ld mv\n",val);

		}
	}
	else {
		val = val*(vref)/4095;
		printf(" battery val is %ld mv\n",val);
	}

	adc_close_channel(channel);

	power_off_adc1();

	return val;
}
