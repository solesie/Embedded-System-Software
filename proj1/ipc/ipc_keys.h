#ifndef IPC_KEYS_H
#define IPC_KEYS_H

/* define System-V5 key_t value */
enum IpcKey {
	/* stoage table semaphore */
	KEY_T_SEM_STORAGE_TABLE = 0x1234,
	
	/* shared memory semaphores */
	KEY_T_SEM_SHM_INPUT,
	KEY_T_SEM_SHM_OUTPUT,

	/* shared memory */
	KEY_T_SHM_INPUT,
	KEY_T_SHM_OUTPUT,

	/* message queue */
	KEY_T_MSG_QUEUE
};

#endif
