# Embedded System Software [CSE4116]

## Goal

Implement a simple stopwatch driver and user application using module programming, device driver implementation, interrupts, top half, and bottom half.

## Build test application(./app)
### make
build
### make push
fusing
### usage
./app

## Build timer device driver(./module)
### make
build
### make push
fusing
### usage
Make sure to locate app file and stopwatch.ko in same location.

## Directories
### app
Stopwatch device driver test application
### module
- stopwatch_ctrl.c 

    Implementation the core logic of the stopwatch.     
    *_wq() functions are resiponsible for Bottom-Half and use single thread workqueue.

- stopwatch_driver.c    

    Implementing file operations for the stopwatch device driver module.

- logging.h     

    Log(printk) utility.

## Device driver info
- major number

    242

- minor number

    0

- device file location

    /dev/stopwatch

- ioctl command

    ```c
    #define IOCTL_RUN _IO(STOPWATCH_MAJOR, 0)
    ```