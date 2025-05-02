#ifndef DEVICE_DRIVER_H
#define DEVICE_DRIVER_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    DEV_SUCCESS = 0,
    DEV_ERROR = -1,
    DEV_NO_DEVICE = -2,
    DEV_BUSY = -3,
    DEV_INVALID = -4
} dev_status_t;

// Opaque handle to a device instance
typedef struct device device_t;

typedef struct {
    const char *name;
    int (*init)(device_t *dev);
    int (*open)(device_t *dev);
    ssize_t (*read)(device_t *dev, void *buf, size_t count);
    ssize_t (*write)(device_t *dev, const void *buf, size_t count);
    int (*close)(device_t *dev);
    int (*ioctl)(device_t *dev, int request, void *arg);
} device_driver_t;

// Device registry API
int register_device(const device_driver_t *driver);
int unregister_device(const char *name);
device_t *get_device(const char *name);

#endif // DEVICE_DRIVER_H

