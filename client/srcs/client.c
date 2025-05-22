#include "client.h"

struct hostent *he;
int sockfd;

int main(int argc, char *argv[])
{
	sigset_t sigset;

	sigfillset(&sigset);
	sigdelset(&sigset, SIGINT);
	sigprocmask(SIG_SETMASK, &sigset, NULL);

	struct sockaddr_in server_addr;

	if (argc != 3)
	{
		fprintf(stderr, "usage: ./client <Hostname or IP> <port> \n");
		exit(1);
	}

	if ((he = gethostbyname(argv[1])) == NULL)
	{
		perror("gethostbyname");
		exit(1);
	}
	
	int port = atoi(argv[2]);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *((struct in_addr*)he->h_addr);

	printf("[ %s ] \n", (char*)inet_ntoa(server_addr.sin_addr));
	memset(&(server_addr.sin_zero), '\0', 8);
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("connect");
		exit(1);	
	}

	while (1)
	{
		printf("* * * Press the number of the option * * *\n");
		printf("1. LED  |  2. Buzzer  |  3. cds  |  4. segment  |  5. exit\n");
		fputs(">> ", stdout);

		int opt;
		scanf("%d", &opt);
		getchar();  // 버퍼에 남아 있는 개행 제거

		char data[64];
		memset(data, 0, sizeof(data));

		switch (opt)
		{
			case 1: // LED
			{
				char input[16];
				printf("Select brightness ([1] LOW, [2] MID, [3] HIGH, [4] OFF):\n");
				fputs(">> ", stdout);
				fgets(input, sizeof(input), stdin);
				input[strcspn(input, "\n")] = 0; // 개행 제거

				if (strcasecmp(input, "1") == 0)
					strcpy(data, "led_10 ");
				else if (strcasecmp(input, "2") == 0)
					strcpy(data, "led_40 ");
				else if (strcasecmp(input, "3") == 0)
					strcpy(data, "led_100 ");
				else if (strcasecmp(input, "4") == 0)
					strcpy(data, "led_0 ");
				else {
					printf("Invalid input. Try again.\n\n");
					continue;
				}
				break;
			}

			case 2: // Buzzer
			{
				char input[16];
				printf("[1] ON  |  [0] OFF\n");
				fputs(">> ", stdout);
				fgets(input, sizeof(input), stdin);
				input[strcspn(input, "\n")] = 0; // 개행 제거

				if (strcasecmp(input, "1") == 0)
					strcpy(data, "buzz_on ");
				else if (strcasecmp(input, "0") == 0)
					strcpy(data, "buzz_off ");
				else {
					printf("Invalid input. Try again.\n\n");
					continue;
				}
				break;
			}

			case 3: // CDS
				strcpy(data, "pr ");
				break;

			case 4: // Segment
			{
				char input[4];
				printf("Input a digit (0~9):\n");
				fputs(">> ", stdout);
				fgets(input, sizeof(input), stdin);
				input[strcspn(input, "\n")] = 0; // 개행 제거

				if (strlen(input) == 1 && isdigit(input[0])) 
				{
					snprintf(data, sizeof(data), "seg_%c ", input[0]);
				} 
				else 
				{
					printf("Invalid input. Try again.\n\n");
					continue;
				}
				break;
			}

			case 5: // Exit
				printf("client shutdown\n");
				close(sockfd);
				exit(0);

			default:
				printf("Invalid menu option. Try again.\n\n");
				continue;
		}

		createMessage(data);
	}

	close(sockfd);
	return 0;	
}
