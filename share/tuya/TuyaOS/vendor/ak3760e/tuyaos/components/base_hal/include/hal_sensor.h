/*
 * Copyright (C) 2021 Tuya Inc.
 */

/*!
 * @file
 * @brief hal interface for sensor
 */
#ifndef __HAL_SENSOR_H__
#define __HAL_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long int hal_sensor_t;

enum hal_sensor_type {
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
};

/**
 * @brief Initialize all sensors in the system.
 * @return 0 on success, error code on failure.
 */
int hal_sensors_init(void);

/**
 * @brief De-initialize all sensors in the system.
 * @return 0 on success, error code on failure.
 */
int hal_sensors_exit(void);

/**
 * @brief Get sensor handle by type and index.
 * @param type sensor type
 * @param index sensor index
 * @return sensor handle on success, 0 on failure.
 */
hal_sensor_t hal_sensor_get(enum hal_sensor_type type, int index);

/**
 * @brief Release the sensor handle
 * @param sensor sensor handle
 */
void hal_sensor_put(hal_sensor_t sensor);

/**
 * @brief Read sensor data
 * @param sensor sensor handler
 * @return sensor data on success, negative value on failure.
 */
float hal_sensor_read(hal_sensor_t sensor);


#ifdef __cplusplus
}
#endif

#endif
