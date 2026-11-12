#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "product_test_gpio.h"
#include "product_test_ircut.h"
#include "ptal_ircut.h"
#include "ptal_gpio.h"
#include "tb_log.h"

static int sg_ircut_init_flag = 0;

static int _product_test_ircut_init(void) 
{
    int iret = 0;
    int ircut1_gpio,ircut2_gpio;
    int ircut1_level,ircut2_level;

    if(sg_ircut_init_flag)
        return 0;

    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN1,&ircut1_gpio);
    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN2,&ircut2_gpio);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN1,&ircut1_level);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN2,&ircut2_level);

    TB_LOGD("ircut gpio %d,%d",ircut1_gpio,ircut2_gpio);

    product_test_gpio_init(ircut1_gpio,E_GPIO_OUT,ircut1_level^1);
    product_test_gpio_init(ircut2_gpio,E_GPIO_OUT,ircut2_level^1);

    sg_ircut_init_flag = 1;

    return iret;
}

int __attribute__((weak)) ptal_test_ircut_day(void) 
{
    int iret = 0;
    int ircut1_gpio,ircut2_gpio;
    int ircut1_level,ircut2_level;

    TB_LOGD();

    _product_test_ircut_init();

    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN1,&ircut1_gpio);
    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN2,&ircut2_gpio);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN1,&ircut1_level);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN2,&ircut2_level);

    if(iret ) {
        TB_LOGE("ircut init fail"); 
        return -1;  
    }

    product_test_gpio_set_value(ircut1_gpio,ircut1_level);
    product_test_gpio_set_value(ircut2_gpio,ircut2_level^1);
    
    return 0;
}

int __attribute__((weak)) ptal_ircut_night(void) 
{
    int iret = 0;
    int ircut1_gpio,ircut2_gpio;
    int ircut1_level,ircut2_level;

    TB_LOGD();

    _product_test_ircut_init();

    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN1,&ircut1_gpio);
    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN2,&ircut2_gpio);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN1,&ircut1_level);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN2,&ircut2_level);

    if(iret ) {
        TB_LOGE("ircut init fail"); 
        return -1;  
    }

    product_test_gpio_set_value(ircut1_gpio,ircut1_level^1);
    product_test_gpio_set_value(ircut2_gpio,ircut2_level);

    return 0;
}

int __attribute__((weak)) ptal_ircut_off(void) 
{
    int iret = 0;
    int ircut1_gpio,ircut2_gpio;
    int ircut1_level,ircut2_level;

    _product_test_ircut_init();

    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN1,&ircut1_gpio);
    iret |= product_test_ircut_get_gpio_num(E_IRCUT_PIN2,&ircut2_gpio);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN1,&ircut1_level);
    iret |= product_test_ircut_get_gpio_level(E_IRCUT_PIN2,&ircut2_level);

    if(iret ) {
        TB_LOGE("ircut init fail"); 
        return -1;  
    }

    product_test_gpio_set_value(ircut1_gpio,ircut1_level^1);
    product_test_gpio_set_value(ircut2_gpio,ircut2_level^1);

    return 0;
}
