/*
 * Copyright (C) 2021 Tuya Inc.
 */

/*!
 * @file
 * @brief hal interface for sensor
 */
#ifndef __TKL_SENSOR_H__
#define __TKL_SENSOR_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long int TUYA_SENSOR_T;

typedef enum {
    SENSOR_VOLTAGE,
    SENSOR_CURRENT,
    SENSOR_POWER,
    SENSOR_ACCEL,
    SENSOR_ANGL_VEL,
    SENSOR_MAGN,
    SENSOR_LIGHT,
    SENSOR_INTENSITY,
    SENSOR_PROXIMITY,
    SENSOR_TEMP,
    SENSOR_INCLI,
    SENSOR_ROT,
    SENSOR_ANGL,
    SENSOR_TIMESTAMP,
    SENSOR_CAPACITANCE,
    SENSOR_ALTVOLTAGE,
    SENSOR_CCT,
    SENSOR_PRESSURE,
    SENSOR_HUMIDITYRELATIVE,
    SENSOR_ACTIVITY,
    SENSOR_STEPS,
    SENSOR_ENERGY,
    SENSOR_DISTANCE,
    SENSOR_VELOCITY,
    SENSOR_CONCENTRATION,
    SENSOR_RESISTANCE,
    SENSOR_QUATERNION,
}TUYA_SENSOR_TYPE_E;

/**
 * @brief Initialize all sensors in the system.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_sensors_init(VOID_T);

/**
 * @brief De-initialize all sensors in the system.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_sensors_exit(VOID_T);


/**
 * @brief Get sensor handle by type and index.
 * @param type sensor type
 * @param index sensor index
 * @return sensor value.
 */
TUYA_SENSOR_T tkl_sensor_get(TUYA_SENSOR_TYPE_E type, INT_T index);

/**
 * @brief Release the sensor handle
 * @param sensor sensor handle
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_sensor_put(TUYA_SENSOR_T sensor);

/**
 * @brief Read sensor data
 * @param sensor sensor handler
 * @param read value
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_sensor_read(TUYA_SENSOR_T sensor, FLOAT_T *value);

#ifdef __cplusplus
}
#endif

#endif
