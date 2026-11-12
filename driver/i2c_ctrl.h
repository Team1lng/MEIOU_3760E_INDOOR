#ifndef __I2C_CTRL_H__
#define __I2C_CTRL_H__
typedef struct i2c_context i2c_context;

int i2c_device_init(void);

i2c_context* i2c_device_open(int index);

int i2c_device_write(i2c_context* ctx, char addres, int reg, int reg_len, char* data, int datasize);

int i2c_device_read(i2c_context* ctx, char addres, int reg, int reg_len, char* data, int datasize);
#if 0
int i2c_device_write_ex(i2c_context* ctx, char addres, char* reg, char* data, int len);

int i2c_device_read_ex(i2c_context* ctx, char addres, char* reg, char* data, int len);
#endif

int i2c_device_close(i2c_context* ctx);
#endif
