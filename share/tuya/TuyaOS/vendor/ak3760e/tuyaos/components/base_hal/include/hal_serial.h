/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_SERIAL_H__
#define __HAL_SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_SERIAL_P_NONE,      /* no parity */
    HAL_SERIAL_P_EVEN,      /* even parity */
    HAL_SERIAL_P_ODD,       /* odd parity */
} hal_serial_parity_e;

typedef enum {
    HAL_SERIAL_STOP_1 = 1,  /* 1 stop bit */
    HAL_SERIAL_STOP_2,      /* 2 stop bits */
    HAL_SERIAL_STOP_15,     /* 1.5 stop bits */
} hal_serial_stopbits_e;

typedef enum {
    HAL_SERIAL_FC_NONE,     /* no flow control */
    HAL_SERIAL_FC_RTSCTS,   /* RTS/CTS flow control */
    HAL_SERIAL_FC_XONXOFF,  /* XON/XOFF flow control */
    HAL_SERIAL_FC_DTRDSR,   /* DTR/DSR flow control */
} hal_serial_flowctrl_e;

typedef struct {
    int baud;       /* baud rate */
    int databits;   /* 5,6,7,8 bits */
    hal_serial_parity_e parity;
    hal_serial_stopbits_e stopbits;
    hal_serial_flowctrl_e flowctrl;
} hal_serial_attr_t;


#ifdef __cplusplus
}
#endif

#endif
