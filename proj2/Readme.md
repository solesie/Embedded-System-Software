# Embedded System Software [CSE4116]

## Goal

Develop timer device driver and related test application.

## Build test application(./app)
### make
build
### make push
fusing

## Build timer device driver(./module)
### make
build
### make push
fusing

## Directories
### app
Timer device driver test application
### module
- fpga_ctrl.c 

    Abstraction of FPGA device I/O for the timer driver.
- ioctl_timer_ctrl.c    

    Implementation the core logic of the timer device driver that operates according to ioctl commands.
- timer_driver.c    

    Implementing file operations for the timer device driver module.

- logging.h     

    Log(printk) utility.