#include "server.h"

/* 스레드 처리를 위한 함수 */
static void *clnt_connection(void *arg);

int led_light = 0;
int pr_light;
char proj_dir[128];
pthread_mutex_t led_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buzz_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pr_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t seg_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv)
{
    struct sigaction sa; /* 시그널 처리를 위한 시그널 액션 */
    struct rlimit rl;
    int fd0, fd1, fd2, i;
    pid_t pid;

    /* 프로그램을 시작할 때 서버를 위한 포트 번호를 입력받는다. */
    if(argc!=2) {
        printf("usage: %s <port>\n", argv[0]);
        return -1;
    }


    /* 파일 생성을 위한 마스크를 0으로 설정 */
    umask(0);

    /* 사용할 수 있는 최대의 파일 디스크립터 수 얻기 */
    if(getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        perror("getlimit()");
    }

    if((pid = fork()) < 0) {
        perror("error()");
    } else if(pid != 0) { /* 부모 프로세스는 종료한다. */
        return 0;
    }

    /* 터미널을 제어할 수 없도록 세션의 리더가 된다. */
    setsid();

    /* 터미널 제어와 관련된 시그널을 무시한다. */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction() : Can't ignore SIGHUP");
    }

    getcwd(proj_dir, 128);
    /* 프로세스의 워킹 디렉터리를 ‘/’로 설정한다. */
    if(chdir("/") < 0) {
        perror("cd()");
    }

    /* 프로세스의 모든 파일 디스크립터를 닫는다. */
    if(rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }

    for(i = 0; i < rl.rlim_max; i++) {
        close(i);
    }

    /* 파일 디스크립터 0, 1과 2를 /dev/null로 연결한다. */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    /* 로그 출력을 위한 파일 로그를 연다. */
    openlog(argv[1], LOG_CONS, LOG_DAEMON);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        return -1;
    }

    /* 로그 파일에 정보 수준의 로그를 출력한다. */
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

    /* 서버를 위한 소켓을 생성한다. */
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if(ssock == -1) {
        perror("socket()");
        return -1;
    }

    /* 입력받는 포트 번호를 이용해서 서비스를 운영체제에 등록한다. */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = (argc != 2)?htons(8000):htons(atoi(argv[1]));
    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1) {
        perror("bind()");
        return -1;
    }
    
    /* 최대 10대의 클라이언트의 동시 접속을 처리할 수 있도록 큐를 생성한다. */
    if(listen(ssock, 10) == -1) {
        perror("listen()");
        return -1;
    }


    while(1) {
        /* 데몬 프로세스로 해야 할 일을 반복 수행 */
        char mesg[BUFSIZ];
        int csock;

        /* 클라이언트의 요청을 기다린다. */
        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr*)&cliaddr, &len);

        /* 네트워크 주소를 문자열로 변경 */
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client IP : %s:%d\n", mesg, ntohs(cliaddr.sin_port));

        /* 클라이언트의 요청이 들어오면 스레드를 생성하고 클라이언트의 요청을 처리한다. */
        pthread_create(&thread, NULL, clnt_connection, &csock);
        pthread_detach(thread);
    }

    /* 시스템 로그를 닫는다. */
    closelog();
    return 0;
}

void *clnt_connection(void *arg)
{    
    /* 스레드를 통해서 넘어온 arg를 int 형의 파일 디스크립터로 변환한다. */
    int csock = *((int*)arg);
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[BUFSIZ], type[BUFSIZ];
    char filename[BUFSIZ], *ret;

    /* 파일 디스크립터를 FILE 스트림으로 변환한다. */
    clnt_read = fdopen(csock, "r");
    clnt_write = fdopen(dup(csock), "w");

    /* 한 줄의 문자열을 읽어서 reg_line 변수에 저장한다. */
    while (fgets(reg_line, BUFSIZ, clnt_read))
    {
        /* reg_line 변수에 문자열을 화면에 출력한다. */
        fputs(reg_line, stdout);
    
        /* ' ' 문자로 reg_line을 구분해서 요청 라인의 내용(메소드)를 분리한다. */
        ret = strtok(reg_line, "/ ");
        strcpy(method, (ret != NULL)?ret:"");
        if(strcmp(method, "POST") == 0) { 		/* POST 메소드일 경우를 처리한다. */
            sendOk(clnt_write); 			/* 단순히 OK 메시지를 클라이언트로 보낸다. */
            goto END;
        } else if(strcmp(method, "GET") != 0) {	/* GET 메소드가 아닐 경우를 처리한다. */
            sendError(clnt_write); 			/* 에러 메시지를 클라이언트로 보낸다. */
            goto END;
        }
    
        ret = strtok(NULL, " "); 			/* 요청 라인에서 경로(path)를 가져온다. */
        strcpy(filename, (ret != NULL)?ret:"");
    
        // TODO: 파일이름 아니고 led on, off 를 받아야함.
        if(filename[0] == '/') 
        { 			/* 경로가 '/'로 시작될 경우 /를 제거한다. */
            memmove(filename, filename + 1, strlen(filename));
        }
    
        pthread_t tid;

        char *qmark = strchr(filename, '?');
        if (qmark != NULL)
            *qmark = '\0';  // ?부터 자름

        if (strcmp(filename, "") == 0 || strcmp(filename, "index.html") == 0)
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
            softToneStop(SPKR);
            // softToneWrite(SPKR, 0);
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
        /* 메시지 헤더를 읽어서 화면에 출력하고 나머지는 무시한다. */
        do {
            fgets(reg_line, BUFSIZ, clnt_read);
            fputs(reg_line, stdout);
            strcpy(reg_buf, reg_line);
//          char* buf = strchr(reg_buf, ':');
        } while(strncmp(reg_line, "\r\n", 2)); 	/* 요청 헤더는 ‘\r\n’으로 끝난다. */
        
        /* 파일의 이름을 이용해서 클라이언트로 파일의 내용을 보낸다. */
        sendData(clnt_write, type, filename);
    }
    
END:
    fclose(clnt_read); 				/* 파일의 스트림을 닫는다. */
    fclose(clnt_write);
    pthread_exit(0); 				/* 스레드를 종료시킨다. */

    return (void*)NULL;
}
    
int sendData(FILE* fp, char *ct, char *filename)
{
    /* 클라이언트로 보낼 성공에 대한 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n";
    char end[ ] = "\r\n"; 			/* 응답 헤더의 끝은 항상 \r\n */
    char buf[BUFSIZ];
    int fd, len;

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);

    fd = open(filename, O_RDWR); 		/* 파일을 연다. */
    do {
        len = read(fd, buf, BUFSIZ); 		/* 파일을 읽어서 클라이언트로 보낸다. */
        fputs(buf, fp);
    } while(len == BUFSIZ);

    close(fd); 					/* 파일을 닫는다. */
    fflush(fp);
    return 0;
}
    
   
void sendOk(FILE* fp)
{
    /* 클라이언트에 보낼 성공에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n\r\n";

    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}
    
void sendError(FILE* fp)
{
    /* 클라이언트로 보낼 실패에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 400 Bad Request\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n";
    char cnt_len[ ] = "Content-Length:1024\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n\r\n";

    /* 화면에 표시될 HTML의 내용 */
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

    // LED Control
    fprintf(fp,
    "<div class='section'><h2>LED Control</h2>"
    "<form action='/led_0' method='get'><button>LED OFF</button></form>"
    "<form action='/led_10' method='get'><button>LED LOW</button></form>"
    "<form action='/led_40' method='get'><button>LED MID</button></form>"
    "<form action='/led_100' method='get'><button>LED HIGH</button></form>"
    "</div>");

    // Buzz
    fprintf(fp,
    "<div class='section'><h2>Music Buzzer</h2>"
    "<form action='/buzz_on' method='get'><button>Buzz ON</button></form>"
    "<form action='/buzz_off' method='get'><button>Buzz OFF</button></form>"
    "</div>");

    // 7-segment
    fprintf(fp, "<div class='section'><h2>7-Segment Display</h2>");
    for (int i = 0; i <= 9; i++) {
        fprintf(fp,
            "<form action='/seg_%d' method='get'><button>%d</button></form>",
            i, i);
    }
    fprintf(fp, "</div>");

    // CDS
    fprintf(fp,
    "<div class='section'><h2>cds sensor</h2>"
    "<form action='/pr' method='get'><button>Measure Light</button></form>");
    fprintf(fp, "<p>cds measured value : <strong>%d</strong></p>", pr_light);
    fprintf(fp, "</div>");
}
