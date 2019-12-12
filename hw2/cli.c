#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>


#define BUFF_SIZE 4096
#define PORT 6667

char buff[BUFF_SIZE];
char msg[BUFF_SIZE];
char op[BUFF_SIZE];

void *thread_listen(void *args)
{
	int fd = *((int *)args);

	while(1)
	{
		memset(buff, 0, sizeof(buff));
		read(fd, buff, BUFF_SIZE);

		if(strlen(buff) >= 1)printf("******Server: \n%s\n\n", buff);

		if(strncmp("bye", buff, 3) == 0)
		{
			break;
		}

		if(strncmp("Play", buff, 4) == 0)
		{
			fgets(msg, BUFF_SIZE, stdin);
			write(fd, msg, strlen(msg));
		}
	}

	pthread_exit(NULL);
}

void *thread_write(void *args)
{
	int fd = *((int *)args);


	while(1)
	{
		fgets(op, BUFF_SIZE, stdin);

		write(fd, op, strlen(op));
		if(strncmp("logout", op, 6) == 0)
		{
			break;
		}
	}

	pthread_exit(NULL);
}

int main()
{
	int cliSocket;
	struct sockaddr_in svrAddr;
	socklen_t addr_size;

	cliSocket = socket(PF_INET, SOCK_STREAM, 0);
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(PORT);
	svrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(svrAddr.sin_zero, 0, sizeof(svrAddr.sin_zero));

	addr_size = sizeof(svrAddr);
	connect(cliSocket, (struct sockaddr *) &svrAddr, addr_size);

	pthread_t tid_listen, tid_write;

	pthread_create(&tid_listen, NULL, thread_listen, &cliSocket);
	pthread_create(&tid_write, NULL, thread_write, &cliSocket);


	pthread_join(tid_listen, NULL);
	pthread_join(tid_write, NULL);

	close(cliSocket);

	return 0;
}
