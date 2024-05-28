#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#define STOPWATCH_MAJOR 242
#define STOPWATCH_NAME "stopwatch"
#define IOCTL_RUN _IO(STOPWATCH_MAJOR, 0)

static void sigint_handler(int sig){
	fprintf(stderr, "[app] To exit normally, must press RESET button and wait.\n");
}

static void insmod(){
	pid_t pid = fork();
	if(pid == 0){
		execl("/system/bin/sh", "sh", "-c", "insmod stopwatch.ko", NULL);
        // fail
		perror("execl");
        _exit(1); 
	}
	int status;
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		fprintf(stderr, "[app] execl: insmod stopwatch.ko failed.\n");
	}
}

int main(int argc, char** argv){
	(void)signal(SIGINT, sigint_handler);

	insmod();
	int fd = open("/dev/stopwatch", O_RDWR);
	if(fd == -1) return 1;

	if(ioctl(fd, IOCTL_RUN) < 0){
		close(fd);
		// assert not happen
		return 1;
	}

	close(fd);
	return 0;	
}
