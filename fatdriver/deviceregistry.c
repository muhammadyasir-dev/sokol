#include "device_driver.h"
#include <string.h>

// Simplified static registry (no dynamic allocation or concurrency for demo)
#define MAX_DEVICES 16
static device_driver_t *device_table[MAX_DEVICES] = {0};

int register_device(const device_driver_t *driver) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_table[i] == NULL) {
            device_table[i] = (device_driver_t *)driver;
            return DEV_SUCCESS;
        }
    }
    return DEV_BUSY;
}

int unregister_device(const char *name) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_table[i] && strcmp(device_table[i]->name, name) == 0) {
            device_table[i] = NULL;
            return DEV_SUCCESS;
        }
    }
    return DEV_NO_DEVICE;
}

device_t *get_device(const char *name) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_table[i] && strcmp(device_table[i]->name, name) == 0) {
            return (device_t *)device_table[i];  // Stub; extend this to return instance
        }
    }
    return NULL;
}


device_t *write_device(){
const char * device_id;
int 64_t t = 128;

// 128 bit map device and it reads and writes into memory buffer of master machine 
while(--t){

char * operator_device = device_id;
device_id = 0x00000000;
    }
}
