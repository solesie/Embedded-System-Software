/*
 * The IPC payload and the process's own logic use this enum.
 */

#ifndef MODE_H
#define MODE_H

#define MODE_CNT 3

enum mode{
	PUT,
	GET,
	MERGE
};

#endif // MODE_H
