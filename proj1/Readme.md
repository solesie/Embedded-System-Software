# Embedded System Software [CSE4116]

## Goal

Creating a Simple Key-Value Store using Device Control and IPC (Inter-Process Communication) in C

## Build
### make
build
### make push
fusing

## Directories
### common
Define globally used entities
### device
Abstracted device input/output controller
### ipc
Define structures for IPC (semaphore, shared memory, message queue) functionalities
### ipc/payload
Standardize values passed through IPC
### process
Define main/io/merge processes