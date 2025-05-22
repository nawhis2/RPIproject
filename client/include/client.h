#ifndef __CLIENT_H__
#define __CLIENT_H__

#define SERVER_IP 192.168.0.105

#define MAXDATASIZE 512 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

int createMessage(char *cmd);
int communicateWithServer(char *message);

#endif