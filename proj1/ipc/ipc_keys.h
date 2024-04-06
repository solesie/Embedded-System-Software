/*
 * define System-V5 key_t value
 */

#ifndef IPC_KEYS_H
#define IPC_KEYS_H

enum sem_keys {
	_KEY_T_SEM_START = 0x1000,

	/* stoage table semaphore */
	KEY_T_SEM_ST,
	
	/* shared memory semaphores */
	KEY_T_SEM_SHM_INPUT,
	KEY_T_SEM_SHM_OUTPUT,
	KEY_T_SEM_SHM_MT,

	_KEY_T_SEM_END
};

enum shm_keys {
	_KEY_T_SHM_START = 0x2000,

	/* shared memory */
	KEY_T_SHM_INPUT,
	KEY_T_SHM_OUTPUT,
	KEY_T_SHM_MT,

	_KEY_T_SHM_END
};

enum msgq_keys {
	_KEY_T_MSGQ_START = 0x3000,

	/* message queue */
	KEY_T_MSGQ,

	_KEY_T_MSGQ_END
};

#endif // IPC_KEYS_H
