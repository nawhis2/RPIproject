#include "server.h"

static void *clnt_connection(void *arg);

int led_light = 0;
int pr_light;
int music_stop = 0;

char proj_dir[128];

pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buzz_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pr_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t seg_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv)
{
    struct sigaction sa;
    struct rlimit rl;
    int fd0, fd1, fd2, i;
    pid_t pid;

    if(argc!=2) {
        printf("usage: %s <port>\n", argv[0]);
        return -1;
    }

    umask(0);
    if(getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        perror("getlimit()");
    }

    if((pid = fork()) < 0) {
        perror("error()");
    } else if(pid != 0) {
        return 0;
    }

    setsid();

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction() : Can't ignore SIGHUP");
    }

    getcwd(proj_dir, 128);
    if(chdir("/") < 0) {
        perror("cd()");
    }

    if(rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }

    for(i = 0; i < rl.rlim_max; i++) {
        close(i);
    }

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog(argv[1], LOG_CONS, LOG_DAEMON);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        return -1;
    }

    syslog(LOG_INFO, "Daemon Process");

    int ssock;
    pthread_t thread;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int len;

    if (wiringPiSetupGpio() == -1)
        return -1;
    pinMode(GPIO17, OUTPUT);
    softPwmCreate(GPIO17, 0, 100);
    softPwmWrite(GPIO17, 0);
    softToneCreate(SPKR);

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if(ssock == -1) {
        perror("socket()");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = (argc != 2)?htons(8000):htons(atoi(argv[1]));
    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1) {
        perror("bind()");
        return -1;
    }
    
    if(listen(ssock, 10) == -1) {
        perror("listen()");
        return -1;
    }


    while(1) {
        char mesg[BUFSIZ];
        int csock;

        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr*)&cliaddr, &len);

        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client IP : %s:%d\n", mesg, ntohs(cliaddr.sin_port));

        pthread_create(&thread, NULL, clnt_connection, &csock);
        pthread_detach(thread);
    }

    closelog();
    return 0;
}

void *clnt_connection(void *arg)
{    
    int csock = *((int*)arg);
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[BUFSIZ], type[BUFSIZ];
    char filename[BUFSIZ], *ret;

    clnt_read = fdopen(csock, "r");
    clnt_write = fdopen(dup(csock), "w");

    while (fgets(reg_line, BUFSIZ, clnt_read))
    {
        fputs(reg_line, stdout);
    
        ret = strtok(reg_line, "/ ");
        strcpy(method, (ret != NULL)?ret:"");
        if(strcmp(method, "POST") == 0) { 		
            sendOk(clnt_write); 			
            goto END;
        } else if(strcmp(method, "GET") != 0) {
            sendError(clnt_write); 			
            goto END;
        }
    
        ret = strtok(NULL, " "); 	
        strcpy(filename, (ret != NULL)?ret:"");
    
        if(filename[0] == '/') 
            memmove(filename, filename + 1, strlen(filename));
        
        pthread_t tid;

        char *qmark = strchr(filename, '?');
        if (qmark != NULL)
            *qmark = '\0';

        if (strcmp(filename, "") == 0 || strcmp(filename, "index_html") == 0)
        {}
        else if (strncmp(filename, "led_", 3) == 0)
        {
            pthread_create(&tid, NULL, led_thread, &filename[4]);
            pthread_join(tid, NULL);
        }
        else if (strncmp(filename, "seg_", 3) == 0)
        {
            pthread_create(&tid, NULL, seg_thread, &filename[4]);
            pthread_detach(tid);
        }
        else if (strcmp(filename, "buzz_on") == 0)
        {
            pthread_create(&tid, NULL, buzz_thread, NULL);
            pthread_detach(tid);
        }
        else if (strcmp(filename, "buzz_off") == 0)
        {
            music_stop = 1;
        }
        else if (strcmp(filename, "pr") == 0)
        {
            pthread_create(&tid, NULL, pr_thread, NULL);
            pthread_join(tid, NULL);
        }
        else if (strcmp(filename, "exit") == 0)
        {
            sendOk(clnt_write);
            goto END;
        }
        else
        {
            sendError(clnt_write);
            continue;
        }
        
        sendMainPage(clnt_write);
        do {
            fgets(reg_line, BUFSIZ, clnt_read);
            fputs(reg_line, stdout);
            strcpy(reg_buf, reg_line);
        } while(strncmp(reg_line, "\r\n", 2));
        
        sendData(clnt_write, type, filename);
    }
    
END:
    fclose(clnt_read);
    fclose(clnt_write);
    pthread_exit(0); 
    return (void*)NULL;
}
    
int sendData(FILE* fp, char *ct, char *filename)
{

    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n";
    char end[ ] = "\r\n"; 
    char buf[BUFSIZ];
    int fd, len;

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);

    fd = open(filename, O_RDWR);
    do {
        len = read(fd, buf, BUFSIZ); 
        fputs(buf, fp);
    } while(len == BUFSIZ);

    close(fd);
    fflush(fp);
    return 0;
}
    
   
void sendOk(FILE* fp)
{
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n\r\n";

    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}
    
void sendError(FILE* fp)
{
    char protocol[ ] = "HTTP/1.1 400 Bad Request\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n";
    char cnt_len[ ] = "Content-Length:1024\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n\r\n";

    char content1[ ] = "<html><head><title>BAD Connection</title></head>";
    char content2[ ] = "<body><font size=+5>Bad Request</font></body></html>";
    printf("send_error\n");

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs(content1, fp);
    fputs(content2, fp);
    fflush(fp);
}

void sendMainPage(FILE* fp)
{
    fputs("HTTP/1.1 200 OK\r\n", fp);
    fputs("Content-Type: text/html\r\n\r\n", fp);

    fprintf(fp,
    "<html><head><title>Welcome</title>"
    "<style>"
    "body { font-family: Arial; text-align: center; padding: 40px; }"
    "h1 { font-size: 32px; margin-bottom: 20px; }"
    ".section { margin: 40px 0; }"
    "form { display: inline-block; margin: 5px; }"
    "button { font-size: 18px; padding: 12px 24px; border-radius: 8px; border: 1px solid #ccc; cursor: pointer; background-color: #f8f8f8; }"
    "button:hover { background-color: #e0e0e0; }"
    "</style></head><body>");

    fprintf(fp,
    "<h1>Welcome to Device Control Dashboard</h1>"
    "<p>Select the function you want!</p>");

    fprintf(fp,
    "<div class='section'><h2>LED Control</h2>"
    "<form action='/led_0' method='get'><button>LED OFF</button></form>"
    "<form action='/led_10' method='get'><button>LED LOW</button></form>"
    "<form action='/led_40' method='get'><button>LED MID</button></form>"
    "<form action='/led_100' method='get'><button>LED HIGH</button></form>"
    "</div>");

    fprintf(fp,
    "<div class='section'><h2>Music Buzzer</h2>"
    "<form action='/buzz_on' method='get'><button>Buzz ON</button></form>"
    "<form action='/buzz_off' method='get'><button>Buzz OFF</button></form>"
    "</div>");

    fprintf(fp, "<div class='section'><h2>7-Segment Display</h2>");
    for (int i = 0; i <= 9; i++) {
        fprintf(fp,
            "<form action='/seg_%d' method='get'><button>%d</button></form>",
            i, i);
    }
    fprintf(fp, "</div>");

    fprintf(fp,
    "<div class='section'><h2>cds sensor</h2>"
    "<form action='/pr' method='get'><button>Measure Light</button></form>");
    fprintf(fp, "<p>cds measured value : <strong>%d</strong></p>", pr_light);
    fprintf(fp, "</div>");
}
