#ifndef SHM_IO_H
#define SHM_IO_H

#include "./payload/shm_input.h"
#include "./payload/shm_output.h"

struct shm_io;

struct shm_io* shm_io_create();
void shm_io_destroy(struct shm_io* p);
void shm_io_write_input(struct shm_io* p, const struct shm_input* input);
void shm_io_read_input(struct shm_io* p, struct shm_input* res);
void shm_io_write_output(struct shm_io* p, const struct shm_output* output);
void shm_io_read_output(struct shm_io* p, struct shm_output* res);

#endif // SHM_IO_H
