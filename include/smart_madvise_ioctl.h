#ifndef _IOCTL_DEMO_H
#define _IOCTL_DEMO_H

#include <linux/ioctl.h> 

#define IOCTL_DEMO_DRIVER_NAME "ioctltest" 

#define IOCTL_DEMO_MAGIC '\x66' 

// #define IOCTL_DEMO_VALSET _IOW(IOCTL_DEMO_MAGIC, 0, struct ioctl_demo_arg) 
// #define IOCTL_DEMO_VALGET _IOR(IOCTL_DEMO_MAGIC, 1, struct ioctl_demo_arg) 
// #define IOCTL_DEMO_VALUPD _IOWR(IOCTL_DEMO_MAGIC, 2, struct ioctl_demo_arg) 
#define IOCTL_DEMO_VALSET_NUM _IOW(IOCTL_DEMO_MAGIC, 3, int)
#define IOCTL_DEMO_VALGET_NUM _IOR(IOCTL_DEMO_MAGIC, 4, int)

#define IOCTL_DEMO_DEVICE_FILE_NAME "ioctltest"
#define IOCTL_DEMO_DEVICE_PATH "dev/ioctltest"

enum ioctl_demo_op {
    IOCTL_DEMO_OP_ADD_VALUE,
    IOCTL_DEMO_OP_MINUS_VALUE,
};

struct ioctl_demo_arg { 
    unsigned int val; 
    enum ioctl_demo_op op;
};



#endif