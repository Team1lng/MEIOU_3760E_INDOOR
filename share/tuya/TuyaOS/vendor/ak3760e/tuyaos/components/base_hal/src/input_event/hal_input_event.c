#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <dirent.h>
#include "hal_event.h"
#include "hal_log.h"

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)

# define TEMP_FAILURE_RETRY(expression) \
  (__extension__                                                              \
    ({ long int __result;                                                     \
       do __result = (long int) (expression);                                 \
       while (__result == -1L && errno == EINTR);                             \
       __result; }))



static int input_epoll_fd = 0;
static int input_device_fd[MAX_DEVICES];
static unsigned int input_device_count = 0;
static struct epoll_event polledevents[MAX_DEVICES];


static int test_bit(size_t bit, unsigned long* array)
{
    return (array[bit/BITS_PER_LONG] & (1UL << (bit % BITS_PER_LONG))) != 0;
}

static int get_input_event(struct epoll_event* poll_event, struct input_event* ev)
{
    if (!poll_event || !ev) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    if (poll_event->events & EPOLLIN) {
        /* TEMP_FAILURE_RETRY use to ignore EINTR */
        int r = TEMP_FAILURE_RETRY(read(poll_event->data.fd, ev, sizeof(*ev)));
        if (r == sizeof(*ev)) {
            return 0;
        }
    }
    hal_log(HAL_LOG_ERR, "%s: read failed errno:%d\n", __func__, -errno);
    return -errno;
}

int hal_input_event_init()
{
    if (input_epoll_fd > 0) {
        hal_log(HAL_LOG_ERR, "%s: hal_put_event has already init\n", __func__);
        return -EPERM;
    }

    input_epoll_fd = epoll_create(MAX_DEVICES);
    if (input_epoll_fd < 0) {
        return -errno;
    }

    DIR* dir = opendir("/dev/input");
    if (dir != NULL) {
        struct dirent* de;
        /* try to open all input device */
        while ((de = readdir(dir))) {
            unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];

            if (strncmp(de->d_name, "event", 5))
                continue;
            int fd = openat(dirfd(dir), de->d_name, O_RDONLY);
            if (fd < 0)
                continue;

            /* Read the evbits of the input device.*/
            if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
                close(fd);
                continue;
            }

            /* We assume that only EV_KEY, EV_REL, and EV_SW event types are ever needed.*/
            if (!test_bit(EV_KEY, ev_bits) && !test_bit(EV_REL, ev_bits) && !test_bit(EV_SW, ev_bits) && !test_bit(EV_ABS, ev_bits)) {
                close(fd);
                continue;
            }

            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = fd;

            if (epoll_ctl(input_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
                close(fd);
                hal_log(HAL_LOG_ERR, "%s: epoll_ctl failed errno:%d\n", __func__, -errno);
                continue;
            }

            input_device_fd[input_device_count] = fd;
            input_device_count++;

            if (input_device_count == MAX_DEVICES)
                break;
        }

        closedir(dir);
    }

    if (!input_device_count) {
        close(input_epoll_fd);
        input_epoll_fd = -1;
        hal_log(HAL_LOG_ERR, "%s: can't find input device\n", __func__);
        return -ENODEV;
    }

    return 0;
}

int hal_input_event_recv(struct input_event* ev, int* count, int timeout)
{
    struct epoll_event* poll_events;
    int get_event_count = 0;
    int i, ret = 0;

    if (!ev || !count) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    poll_events = malloc(sizeof(struct epoll_event) * MAX_DEVICES);

    /* if only received EV_SYN event try again */
    do {
        get_event_count = epoll_wait(input_epoll_fd, poll_events, MAX_DEVICES, timeout);

        if (get_event_count == 0) {
            free(poll_events);
            return -ETIMEDOUT;
        }

        if (get_event_count < 0) {
            hal_log(HAL_LOG_ERR, "%s: epoll_wait failed errno:%d\n", __func__, -errno);
            free(poll_events);
            return -errno;
        }
        /* make sure count value is 0 */
        *count = 0;
        for (i = 0; i < get_event_count; i++) {
            ret = get_input_event(&poll_events[i], &ev[*count]);
            if (ret < 0) {
                /* Ignore EV_SYN event */
                if (ret == -EAGAIN) {
                    continue;
                }
                hal_log(HAL_LOG_ERR, "%s: get_input_event failed ret:%d\n", __func__, ret);
                free(poll_events);
                return ret;
            }
            (*count)++;
        }
    } while(!(*count));

    free(poll_events);
    return ret;
}

void hal_input_event_exit()
{
    while (input_device_count > 0) {
        close(input_device_fd[--input_device_count]);
    }

    close(input_epoll_fd);
	input_epoll_fd = 0;
}
