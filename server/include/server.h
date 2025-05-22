#ifndef __DAEMONWEBSERVER_H__
#define __DAEMONWEBSERVER_H__

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wiringPi.h>
#include <softTone.h>
#include <dlfcn.h>
#include <softPwm.h>
#include <wiringPiI2C.h>
#include <string.h>
#include <dlfcn.h>

// #include <openssl/ssl.h>
// #include <openssl/err.h>
// #include <netinet/in.h>

#define GPIO17 17
#define SPKR 	25 	/* GPIO25 */
#define TOTAL 32		/* 학교종의 전체 계이름의 수 */

typedef struct arg
{
    pthread_mutex_t *mutex;
    void* arg;
} arg_t;

/* 스레드 처리를 위한 함수 */
int sendData(FILE* fp, char *ct, char *filename);
void sendOk(FILE* fp);
void sendError(FILE* fp);
void sendMainPage(FILE* fp);

void* led_thread(void *);
void *seg_thread(void *);
void *buzz_thread(void *);
void *pr_thread(void *);


#endif