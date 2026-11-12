#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "cJSON.h"
#include "list.h"
#include "ledAnimation.h"

#define DEFAULT_LED_CONFIG "/etc/led_config.json"
#define LED_INVALED -1
#define LED_SOLID 0
#define LED_FADE 1

struct led_animation_priv
{
	const char *device;
	cJSON *conf;
	struct list_head animations;
};

struct led_animation
{
	struct list_head list;
	const char *name, *fw, *loop, *pause;
};

static void sysfs_write(const char *sysfs_path, char *value)
{
	FILE *fp;

	if (!sysfs_path || !value)
		return;

	fp = fopen(sysfs_path, "w");
	if (fp) {
		fprintf(fp, "%s", value);
		fclose(fp);
	}
}

static int load_fw(struct led_animation_priv *priv, struct led_animation *animation)
{
	char *sysfs_path, *value;

	if (asprintf(&sysfs_path, "%s/avsux_animation", priv->device) > 0) {
			if (asprintf(&value, "%s %s %s %s",
				animation->name, animation->fw, animation->loop, animation->pause) > 0) {
				sysfs_write(sysfs_path, value);
				free(value);
			}
			free(sysfs_path);
	}

	return 0;
}

static void release_fw(struct led_animation_priv *priv)
{
	char *sysfs_path, *value;

	if (asprintf(&sysfs_path, "%s/avsux_animation", priv->device) > 0) {
			sysfs_write(sysfs_path, "reset");

			free(sysfs_path);
	}
}

static int start_animation(struct led_animation_priv *priv, struct led_animation *animation)
{
	char *sysfs_path;

	if (!animation)
		return -EINVAL;

	if (asprintf(&sysfs_path, "%s/avsux_select", priv->device) > 0) {
			sysfs_write(sysfs_path, animation->name);
			free(sysfs_path);
	}

	return 0;
}

static int led_parse_conf(struct led_animation_priv *priv, const char *filename)
{
	FILE *file = NULL;
	long length = 0;
	char *content = NULL;
	size_t read_chars = 0;
	int ret = -1;
	struct led_animation *led_anim;
	const cJSON *device;
	const cJSON *animations;
	const cJSON *animation;
	cJSON *node;

	if (!filename || !priv)
		goto cleanup;

	/* open in read binary mode */
	file = fopen(filename, "rb");
	if (file == NULL)
		goto cleanup;

	/* get the length */
	if (fseek(file, 0, SEEK_END) != 0)
		goto cleanup;
	length = ftell(file);
	if (length < 0)
		goto cleanup;
	if (fseek(file, 0, SEEK_SET) != 0)
		goto cleanup;

	/* allocate content buffer */
	content = (char *)malloc((size_t)length + sizeof(""));
	if (content == NULL)
		goto cleanup;

	/* read the file into memory */
	read_chars = fread(content, sizeof(char), (size_t)length, file);
	if ((long)read_chars != length)
	{
		free(content);
		content = NULL;
		goto cleanup;
	}
	content[read_chars] = '\0';

	priv->conf = cJSON_Parse(content);
	if (!priv->conf) {
		printf("cJSON_Parse failed!\n");
		goto cleanup;
	}

	device = cJSON_GetObjectItemCaseSensitive(priv->conf, "device");
	if (cJSON_IsString(device))
		priv->device = device->valuestring;

	animations = cJSON_GetObjectItemCaseSensitive(priv->conf, "animations");
	cJSON_ArrayForEach(animation, animations)
	{
		led_anim = malloc(sizeof(struct led_animation));
		if (!led_anim)
			continue;

		INIT_LIST_HEAD(&led_anim->list);
		led_anim->name = animation->string;

		node = cJSON_GetObjectItemCaseSensitive(animation, "fw_path");
		if (cJSON_IsString(node))
			led_anim->fw = node->valuestring;

		node = cJSON_GetObjectItemCaseSensitive(animation, "loop");
		if (cJSON_IsString(node))
			led_anim->loop = node->valuestring;
		else
			led_anim->loop = "1";

		node = cJSON_GetObjectItemCaseSensitive(animation, "pause");
		if (cJSON_IsString(node))
			led_anim->pause = node->valuestring;
		else
			led_anim->pause = "0";

		load_fw(priv, led_anim);
		list_add_tail(&led_anim->list, &priv->animations);
	}

	ret = 0;

cleanup:
	if (file != NULL)
		fclose(file);

	if (content)
		free(content);

	return ret;
}

ledHandle_t LedAnimationInit(const char *conf)
{
	struct led_animation_priv *priv = NULL;
	int ret;

	if (!conf)
		conf = DEFAULT_LED_CONFIG;

	priv = (struct led_animation_priv *)malloc(sizeof(struct led_animation_priv));
	if (!priv)
	{
		printf("Alloc memory failed!\n");
		return 0;
	}

	INIT_LIST_HEAD(&priv->animations);

	ret = led_parse_conf(priv, conf);
	if (ret < 0) {
		printf("Parse %s failed:%d\n", conf, ret);
		return 0;
	}

	return (ledHandle_t)priv;
}

int LedAnimationUpdate(ledHandle_t *handle, const char *animation)
{
	struct led_animation_priv *priv;
	struct led_animation *item;
	int ret = -1;

	if (!handle)
	{
		printf("signalLedAnmationUpdate Fail.\n");
		return -1;
	}
	priv = (struct led_animation_priv *)(*handle);

	list_for_each_entry(item, &priv->animations, list)
	{
		if (!strcmp(item->name, animation))
		{
			ret = start_animation(priv, item);
			break;
		}
	}

	return ret;
}

int LedAnimationRelease(ledHandle_t *handle)
{
	struct led_animation_priv *priv = NULL;
	struct led_animation *animation;
	struct list_head *pos, *p;
	int res;
	void *state;

	if (!handle)
	{
		printf("signalLedAnmationRelease handle is null.\n");
		return -1;
	}

	priv = (struct led_animation_priv *)(*handle);

	list_for_each_safe(pos, p, &priv->animations)
	{
		animation = list_entry(pos, struct led_animation, list);
		list_del(pos);
		free(animation);
	}

	release_fw(priv);
	cJSON_free(priv->conf);
	free(priv);
	priv = NULL;

	return 0;
}
