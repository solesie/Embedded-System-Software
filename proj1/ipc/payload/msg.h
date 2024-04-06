#ifndef MSG_H
#define MSG_H

#define MSG_MAX_LEN 20

/* message */
struct msgbuf {
	long mtype;
	char mtext[MSG_MAX_LEN];
};

#endif // MSG_H
