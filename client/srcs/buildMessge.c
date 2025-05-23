#include "client.h"

extern struct hostent* he;
extern int sockfd;

int createMessage(char *cmd)
{
    char message[256];

    memset(message, 0, 256 * sizeof(char));
    strcat(message, "GET /");
    strcat(message, cmd);
    strcat(message, "HTTP/1.1\r\nHost: ");
    strcat(message, inet_ntoa(*(struct in_addr *)he->h_addr_list[0]));
    strcat(message, "\r\n\r\n");

    printf("%s\n\n", message);
    if (communicateWithServer(message) == -1)
        return -1;
    return 0;
}

int communicateWithServer(char *message)
{
    char recvBuf[8192] = {0};

    int nbytes = send(sockfd, message, strlen(message), 0);
    if (nbytes <= 0)
    {
        perror("send");
        return -1;
    }

    if (recv(sockfd, recvBuf, sizeof(recvBuf) - 1, 0) <= 0)
    {
        perror("recv");
        return -1;   
    }
    printf("[ server ] : %s\n", recvBuf);
    return 0;    
}