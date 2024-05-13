#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#define TIMER_MAJOR 242
#define TIMER_NAME "dev_driver"
#define SET_OPTION _IOW(TIMER_MAJOR, 0, struct ioctl_set_option_arg)
#define COMMAND _IO(TIMER_MAJOR, 1)
#define FND_MAX 4

struct ioctl_set_option_arg{
	unsigned int timer_interval, timer_cnt;
	char timer_init[FND_MAX + 1];
};

static void insmod(){
	pid_t pid = fork();
	if(pid == 0){
		execl("/system/bin/sh", "sh", "-c", "insmod dev_driver.ko", NULL);
        // fail
		perror("execl");
        _exit(1); 
	}
	int status;
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		printf("execl: insmod dev_driver.ko failed.\n");
	}
}

static inline void print_error(){
	printf("Usage: ./app TIMER_INTERVAL TIMER_CNT TIMER_INIT\n\n"
		"Where:\n"
    	"- TIMER_INTERVAL: An integer between 1 and 100, indicating the timer interval.\n"
    	"- TIMER_CNT: An integer between 1 and 100, indicating the number of timer counts.\n"
    	"- TIMER_INIT: A 4-digit string where only one digit is between 1 and 8, and the rest are 0s. For example, 0100, 1000, 0008 are valid inputs.\n\n"
    	"Please ensure all arguments meet the specified criteria.\n\n"
    	"Example:\n"
    	"./app 10 5 0100\n");
}

static int validate_argv12(char* argv12, long* res, long s, long e){
    char *endptr;
    errno = 0;

	// decimal
    long val = strtol(argv12, &endptr, 10);

	// check error
    if (endptr == argv12) return 0;
    if (*endptr != '\0') return 0;
    if ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) return 0; // over or underflow

	if(val < s || val > e) return 0;
	*res = val;
    return 1;
}

static int validate_argv3(char* argv3, char timer_init[FND_MAX + 1]){
	int i;
	int len = strlen(argv3);
    int nz_cnt = 0;

    if (len != FND_MAX) return 0;
	for (i = 0; i < FND_MAX; i++) {
        if (argv3[i] < '0' || argv3[i] > '8') return 0;
        if (argv3[i] != '0') nz_cnt++;
    }
	if(nz_cnt != 1) return 0;

	strcpy(timer_init, argv3);
    return 1;
}

int main(int argc, char** argv){
	if(argc != 4){
		print_error();
		return 1;
	}

	long timer_interval = 0;
	long timer_cnt = 0;
	char timer_init[FND_MAX + 1] = {0, };
	if(!validate_argv12(argv[1], &timer_interval, 1, 100) || !validate_argv12(argv[2], &timer_cnt, 1, 200)){
		print_error();
		return 1;
	}
	if(!validate_argv3(argv[3], timer_init)){
		print_error();
		return 1;
	}
	
	insmod();
	int fd = open("/dev/dev_driver", O_RDWR);

	struct ioctl_set_option_arg arg;
	arg.timer_interval = (unsigned int) timer_interval;
	arg.timer_cnt = (unsigned int) timer_cnt;
	strcpy(arg.timer_init, timer_init);
	ioctl(fd, SET_OPTION, &arg);
	printf("To start the timer, press the reset button.\n");
	read(fd, NULL, 1);
	ioctl(fd, COMMAND);

	close(fd);
	return 0;	
}
