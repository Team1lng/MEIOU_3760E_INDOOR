#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sys_log.h"
#include "list.h"
#include "tkl_sensor.h"

#ifndef TAG 
#define TAG "tkl_sensor"
#endif

/***********************************************
 *             Macro definitions
 ***********************************************/
#define SYSFS_PATH_MAX	        512	
#define SYSFS_NAME_LEN		64
#define SYSFS_SENSOR_DIR	"/sys/bus/iio/devices/"
#define SENSOR_MOD_RAW		"raw"
#define SENSOR_MOD_SCALE	"scale"
#define SENSOR_MOD_OFFSET	"offset"
#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))

/***********************************************
 *             Data structures
 ***********************************************/
struct iio_device {
    char name[SYSFS_NAME_LEN];
    char path[SYSFS_PATH_MAX-256];
    struct list_head list;
    struct list_head attrs;
    struct list_head channels;
};

struct iio_attr {
    char name[SYSFS_NAME_LEN];
    char path[SYSFS_PATH_MAX-256];
    struct list_head list;
};

struct iio_channel {
    char name[SYSFS_NAME_LEN];
    char generic_name[SYSFS_NAME_LEN];
    int index;
    int ref_count;
    int dir;
    float raw;
    float scale;
    float offset;
    TUYA_SENSOR_TYPE_E type;
    struct iio_device *dev;
    struct list_head list;
    pthread_mutex_t mutex;
};

/***********************************************
 *             Global variables
 ***********************************************/
LIST_HEAD(sg_sensors_list);

static char * const sg_iio_direction[] = {
    "in_",
    "out_",
};

static const char * const sg_sensor_type_name[] = {
    [SENSOR_VOLTAGE] = "voltage",
    [SENSOR_CURRENT] = "current",
    [SENSOR_POWER] = "power",
    [SENSOR_ACCEL] = "accel",
    [SENSOR_ANGL_VEL] = "anglvel",
    [SENSOR_MAGN] = "magn",
    [SENSOR_LIGHT] = "illuminance",
    [SENSOR_INTENSITY] = "intensity",
    [SENSOR_PROXIMITY] = "proximity",
    [SENSOR_TEMP] = "temp",
    [SENSOR_INCLI] = "incli",
    [SENSOR_ROT] = "rot",
    [SENSOR_ANGL] = "angl",
    [SENSOR_TIMESTAMP] = "timestamp",
    [SENSOR_CAPACITANCE] = "capacitance",
    [SENSOR_ALTVOLTAGE] = "altvoltage",
    [SENSOR_CCT] = "cct",
    [SENSOR_PRESSURE] = "pressure",
    [SENSOR_HUMIDITYRELATIVE] = "humidityrelative",
    [SENSOR_ACTIVITY] = "activity",
    [SENSOR_STEPS] = "steps",
    [SENSOR_ENERGY] = "energy",
    [SENSOR_DISTANCE] = "distance",
    [SENSOR_VELOCITY] = "velocity",
    [SENSOR_CONCENTRATION] = "concentration",
    [SENSOR_RESISTANCE] = "resistance",
    [SENSOR_QUATERNION] = "quaternion",
};

/***********************************************
 *             Internal functions
 ***********************************************/
static inline int __check_prefix(const char *str, const char *prefix) 
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static inline int __check_postfix(const char *str, const char *postfix) 
{
    return strncmp(str + strlen(str) - strlen(postfix), postfix, strlen(postfix)) == 0;
}

static int __is_file(const char *path)
{
    struct stat astats;

    if (!path)
        return 0;

    if ((lstat(path, &astats)) != 0)
        return 0;

    if (S_ISREG(astats.st_mode))
        return 1;

    return 0;
}

static int __sysfs_get_float(const char *path, float *val)
{
    FILE *fp;
    int ret = 0;

    fp = fopen(path, "r");

    if (!fp)
        return -ENODATA;

    if (fscanf(fp, "%f\n", val) != 1)
        ret = -ENODATA;

    fclose(fp);

    return ret;
}

static int __sysfs_get_string(const char *path, char *val)
{
    FILE *fp;
    int ret = 0;

    fp = fopen(path, "r");

    if (!fp)
        return -ENODATA;

    if (fscanf(fp, "%63s\n", val) != 1)
        ret = -ENODATA;

    fclose(fp);

    return ret;
}

static int __iio_channel_attr_parse(struct iio_channel *channel, char *attr_name)
{
    char *tmp, *idx, *p, *prefix = "";
    int i;

    if (!channel)
        return -ENODEV;

    channel->type = 0;
    channel->ref_count = 0;
    channel->index = 0;
    channel->dir = -1;

    /* extract channal fullname from attr */
    for (i = 0; i < ARRAY_SIZE(sg_iio_direction); i++) {
        if (!strncmp(attr_name, sg_iio_direction[i], strlen(sg_iio_direction[i]))) {
	    prefix = sg_iio_direction[i];
	    channel->dir = i;
	    break;
        }
    }

    snprintf(channel->name, strlen(attr_name) - strlen(prefix) - strlen(SENSOR_MOD_RAW),
        "%s", attr_name + strlen(prefix));

    /* Get channel index */
    tmp = strdup(channel->name);

    if (!tmp)
        return -ENOMEM;

    p = tmp;
    idx = tmp;

    while (*p != '\0') {
        if (!isdigit(*p))
	    idx++;
        p++;
    }

    if (p > idx)
        channel->index = atoi(idx);

    /* Get channel generic_name */
    *idx = '\0';
    strcpy(channel->generic_name, tmp);
    free(tmp);

    /* Get channel type */
    while (!__check_prefix(channel->name, sg_sensor_type_name[channel->type]))
        channel->type++;

    return 0;
}

static float __iio_channel_get_mod_value(struct iio_channel *channel,
				  const char *mod, float def_value)
{
    char path[SYSFS_PATH_MAX];
    float val;

    snprintf(path,SYSFS_PATH_MAX, "%s/%s%s_%s", channel->dev->path,
        channel->dir >= 0 ? sg_iio_direction[channel->dir] : "",
        channel->name, mod);

    if (__sysfs_get_float(path, &val) < 0) {
        /* Try read generic modifier value */
        sprintf(path, "%s/%s%s_%s", channel->dev->path,
            channel->dir >= 0 ? sg_iio_direction[channel->dir] : "",
            channel->generic_name, mod);

        if (__sysfs_get_float(path, &val) < 0) {
	    val = def_value;
        }
    }

    return val;
}

static int __iio_device_init_attrs(struct iio_device *iio)
{
    DIR *dir = NULL;
    struct dirent *dirent = NULL;
    struct iio_attr *attr = NULL;
    char file_path[SYSFS_PATH_MAX];

    if (!iio)
        return -ENODEV;

    dir = opendir(iio->path);

    if (!dir) {
	LOGE(TAG, "Error opening directory %s\n", iio->path);
	return -EINVAL;
    }

    INIT_LIST_HEAD(&iio->attrs);

    while ((dirent = readdir(dir)) != NULL) {
        if (0 == strcmp(dirent->d_name, "."))
	    continue;
	if (0 == strcmp(dirent->d_name, ".."))
	    continue;
	memset(file_path, 0, SYSFS_PATH_MAX);
	strcpy(file_path, iio->path);
	strcat(file_path, "/");
	strcat(file_path, dirent->d_name);

	if (__is_file(file_path)) {
	    attr = calloc(1, sizeof(struct iio_attr));

	    if (!attr)
	        return -ENOMEM;

	    strcpy(attr->path, file_path);
	    strcpy(attr->name, basename(file_path));
	    INIT_LIST_HEAD(&attr->list);
	    list_add_tail(&attr->list, &iio->attrs);
        }
    }
    closedir(dir);

    return 0;
}

static int __iio_device_init_channels(struct iio_device *iio)
{
    struct iio_channel *channel;
    struct iio_attr *attr;

    if (!iio)
	return -ENODEV;

    if (list_empty(&iio->attrs))
        return -EINVAL;

    INIT_LIST_HEAD(&iio->channels);

    list_for_each_entry(attr, &iio->attrs, list) {
        if (__check_postfix(attr->name, SENSOR_MOD_RAW)) {
	    channel = calloc(1, sizeof(struct iio_channel));
	    if (!channel)
	        return -ENOMEM;

	    pthread_mutex_init(&channel->mutex, NULL);

	    pthread_mutex_lock(&channel->mutex);
	    channel->dev = iio;

	    __iio_channel_attr_parse(channel, attr->name);

	    channel->raw = __iio_channel_get_mod_value(channel, SENSOR_MOD_RAW, 1.0f);
	    channel->scale = __iio_channel_get_mod_value(channel, SENSOR_MOD_SCALE, 1.0f);
	    channel->offset = __iio_channel_get_mod_value(channel, SENSOR_MOD_OFFSET, 0.0f);

	    INIT_LIST_HEAD(&channel->list);
	    list_add_tail(&channel->list, &iio->channels);
	    pthread_mutex_unlock(&channel->mutex);
        }
    }

    return 0;
}

static int __iio_device_init(const char *path)
{
    struct iio_device *iio;
    int ret = -EINVAL;
    char file_path[SYSFS_PATH_MAX];

    if (!path)
        return ret;

    iio = calloc(1, sizeof(struct iio_device));
    if (!iio)
        return -ENOMEM;

    strcpy(iio->path, path);
    snprintf(file_path,SYSFS_PATH_MAX,"%s/name", iio->path);
    __sysfs_get_string(file_path, iio->name);

    INIT_LIST_HEAD(&iio->list);

    if (__iio_device_init_attrs(iio) == 0) {
        if (__iio_device_init_channels(iio) == 0) {
	    LOGD(TAG, "Found sensor %s.\n", iio->name);
	    list_add_tail(&iio->list, &sg_sensors_list);
	    ret = 0;
        }
    }

    return ret;
}

static int __sensors_init(void)
{
    DIR *dir = NULL;
    struct dirent *dirent = NULL;
    char file_path[SYSFS_PATH_MAX];

    dir = opendir(SYSFS_SENSOR_DIR);

    if (!dir)
        return -ENODEV;

    while ((dirent = readdir(dir)) != NULL) {
        if (0 == strcmp(dirent->d_name, "."))
            continue;
	if (0 == strcmp(dirent->d_name, ".."))
            continue;
        if (__check_prefix(dirent->d_name, "iio:device")) {
            memset(file_path, 0, SYSFS_PATH_MAX);
            snprintf(file_path,SYSFS_PATH_MAX, "%s%s", SYSFS_SENSOR_DIR, dirent->d_name);

            if (__iio_device_init(file_path) < 0)
                return -EINVAL;
        }
    }

    closedir(dir);

    return 0;
}

static int __sensors_exit(void)
{
    struct iio_device *iio;
    struct iio_attr *attr;
    struct iio_channel *channel;
    struct list_head *pos1, *p1, *pos2, *p2;

    list_for_each_safe(pos1, p1, &sg_sensors_list) {
        iio = list_entry(pos1, struct iio_device, list);
        list_for_each_safe(pos2, p2, &iio->channels) {
            channel = list_entry(pos2, struct iio_channel, list);
	    list_del(pos2);
            free(channel);
        }

        list_for_each_safe(pos2, p2, &iio->attrs) {
            attr = list_entry(pos2, struct iio_attr, list);
            list_del(pos2);
            free(attr);
        }
        list_del(pos1);
        free(iio);
    }

    return 0;
}

TUYA_SENSOR_T __sensor_get( TUYA_SENSOR_TYPE_E type, int index)
{
    struct iio_device *iio;
    struct iio_channel *channel;

    list_for_each_entry(iio, &sg_sensors_list, list) {
        list_for_each_entry(channel, &iio->channels, list) {
             if ((channel->type == type) &&
                 (channel->index == index)) {
                 pthread_mutex_lock(&channel->mutex);
                 channel->ref_count++;
                 pthread_mutex_unlock(&channel->mutex);
                 return (TUYA_SENSOR_T)channel;
            }
        }
    }
    return 0;
}

static int __sensor_put(TUYA_SENSOR_T sensor)
{
    struct iio_channel *channel = (struct iio_channel *)sensor;

    if (!channel)
        return -1;

    pthread_mutex_lock(&channel->mutex);

    if (channel->ref_count)
        channel->ref_count--;

    pthread_mutex_unlock(&channel->mutex);

    return 0;
}

static int  __sensor_read(TUYA_SENSOR_T sensor, float *value)
{
    struct iio_channel *channel = (struct iio_channel *)sensor;

    if (!channel || value == NULL)
        return -1;

    pthread_mutex_lock(&channel->mutex);
    channel->raw = __iio_channel_get_mod_value(channel, SENSOR_MOD_RAW, 1.0f);
    *value = (channel->raw + channel->offset) * channel->scale;
    pthread_mutex_unlock(&channel->mutex);

    return 0; 
}

OPERATE_RET tkl_sensors_init(VOID_T)
{
    OPERATE_RET iret = OPRT_OK;

    if(__sensors_init()) {
        iret = OPRT_COM_ERROR;
    }
    return iret;
}

OPERATE_RET tkl_sensors_exit(VOID_T)
{
    OPERATE_RET iret = OPRT_OK;

    if(__sensors_exit()) {
        iret = OPRT_COM_ERROR;
    }
    return iret;
}

TUYA_SENSOR_T tkl_sensor_get(TUYA_SENSOR_TYPE_E type, INT_T index)
{
    return __sensor_get(type,index);
}

OPERATE_RET tkl_sensor_put(TUYA_SENSOR_T sensor)
{
    OPERATE_RET iret = OPRT_OK;

    if(__sensor_put(sensor)) {
        iret = OPRT_COM_ERROR;
    }
    return iret;
}

OPERATE_RET tkl_sensor_read(TUYA_SENSOR_T sensor, FLOAT_T *value)
{
    float v = 0;
    OPERATE_RET iret = OPRT_COM_ERROR;

    if(sensor == 0 || value == NULL) {
        return OPRT_INVALID_PARM; 
    }

    if(__sensor_read(sensor,&v) == 0) {
        iret = OPRT_OK;
        *value = v;	
    }

    return iret;
}
