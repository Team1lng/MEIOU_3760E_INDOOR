#ifndef _PTAL_GPIO_H_
#define _PTAL_GPIO_H_

#ifdef __cplusplus__
extern "C" {
#endif

enum {
    E_GPIO_OUT,/*gpio output enum*/
    E_GPIO_IN, /*gpio input enum */
};


typedef struct gpio_pram_s
{
    int i;                  // gpio index
    int num;                // gpio physic number
}GPIO_T;

#define MAX_GPIOS_IN_GROUPS (32)
typedef struct  gpio_group_s
{
    int val;                // gpio test value
    int num;                // gpip numbers in one group
    GPIO_T gpio[MAX_GPIOS_IN_GROUPS];         // gpio parm
}GPIO_GROUP_T;

extern GPIO_GROUP_T g_gpio_group[];
extern GPIO_GROUP_T g_gpio_sub_mcu_group[];

extern int g_gpio_array_counts;
extern int g_gpio_sub_mcu_array_counts;

extern int ptal_gpio_init(int num,int direction,int init);
extern int ptal_gpio_set_value(int num,int value); 
extern int ptal_gpio_get_value(int num,int *value);

extern int ptal_gpio_sub_mcu_group_test(GPIO_GROUP_T *group); 
extern int ptal_gpio_group_test(GPIO_GROUP_T *group); 
extern int ptal_gpio_set_direction(int num,int direction,int init);

#ifdef __cplusplus__
}
#endif

#endif
