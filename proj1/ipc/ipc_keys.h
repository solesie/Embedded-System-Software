/*
 * define System-V5 key_t value
 */

#ifndef IPC_KEYS_H
#define IPC_KEYS_H

enum sem_keys {
	_KEY_T_SEM_START = 0x111,
	
	/* shared memory semaphores */
	KEY_T_SEM_SHM_INPUT,
	KEY_T_SEM_SHM_OUTPUT,
	KEY_T_SEM_SHM_DB,

	_KEY_T_SEM_END
};

enum shm_keys {
	_KEY_T_SHM_START = 0x222,

	/* shared memory */
	KEY_T_SHM_INPUT,
	KEY_T_SHM_OUTPUT,
	KEY_T_SHM_DB,

	_KEY_T_SHM_END
};

enum msgq_keys {
	_KEY_T_MSGQ_START = 0x333,

	/* message queue */
	KEY_T_MSGQ,

	_KEY_T_MSGQ_END
};

#endif // IPC_KEYS_H
