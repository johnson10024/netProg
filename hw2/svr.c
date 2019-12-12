#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>

#define BUFF_SIZE 4096
#define PORT 6667

char climsg[BUFF_SIZE];
char buff[BUFF_SIZE];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

char board[10][10];

int users[100] = {0};
int playwith[100] = {0};
int userCount = 0;
char inp[100];

int cand[10];
int curBoard[100];
int boardCount = 0;

void board_init()
{
	for(int i = 0; i < 10; i++)
	{
		strcpy(board[i], ".........");
	}
}

int win(int b)
{
	int result = 0;
	if(board[b][0] == board[b][1] && board[b][1] == board[b][2] && board[b][2] != '.') result = 1;
	if(board[b][3] == board[b][4] && board[b][4] == board[b][5] && board[b][5] != '.') result = 1;
	if(board[b][6] == board[b][7] && board[b][7] == board[b][8] && board[b][8] != '.') result = 1;

	if(board[b][0] == board[b][3] && board[b][3] == board[b][6] && board[b][6] != '.') result = 1;
	if(board[b][1] == board[b][7] && board[b][7] == board[b][4] && board[b][4] != '.') result = 1;
	if(board[b][2] == board[b][5] && board[b][5] == board[b][8] && board[b][8] != '.') result = 1;

	if(board[b][0] == board[b][4] && board[b][4] == board[b][8] && board[b][8] != '.') result = 1;
	if(board[b][2] == board[b][4] && board[b][4] == board[b][6] && board[b][6] != '.') result = 1;

	int isdraw = 1;
	for(int i = 0; i < 9; i++)
	{
		if(board[b][i] == '.')
		{
			isdraw = 0;
			break;
		}
	}

	if(isdraw) return -1;
	return result;
}

void *thread(void *arg)
{
	printf("In Thread %d\n", *((int *)arg));

	int newSocket = *((int *)arg);
	int curUser, aite = -1;

	curUser = newSocket;

	while(1)
	{
		recv(newSocket, climsg, BUFF_SIZE, 0);

		if(strncmp("login", climsg, 5) == 0)
		{
			users[curUser] = 1;
			userCount++;
			sprintf(buff, "Welcome user %d\n", curUser);
			
			write(newSocket, buff, strlen(buff));
		}
		if(strncmp("logout", climsg, 6) == 0)
		{
			users[curUser] = 0;
			userCount--;

			write(newSocket, "bye", 3);
			break;
		}
		if(strncmp("list", climsg, 4) == 0)
		{
			sprintf(buff, "User List:\n\n");

			for(int i = 1; i < 100; i++)
			{
				if(users[i])
				{
					sprintf(buff + strlen(buff), "User %d\n", i);
				}
			}

			write(newSocket, buff, strlen(buff) + 1);
		}
		if(strncmp("playwith", climsg, 8) == 0)
		{
			sscanf(climsg, "%s %d", inp, &aite);
printf("%d %s %d\n", curUser, inp, aite);

			if(playwith[aite] != 0 || aite == curUser)
			{
				write(newSocket, "Rejected!\n", 11);
			}
			else
			{
				sprintf(buff, "%d Accepted\n", aite);
				playwith[aite] = curUser;
				playwith[curUser] = aite;
				curBoard[curUser] = boardCount;
				curBoard[aite] = boardCount++;
				boardCount %= 10;
				cand[curBoard[curUser]] = curUser;

				write(newSocket, buff, strlen(buff));
			}
		}
		if(strncmp("go", climsg, 2) == 0)
		{
			if(playwith[curUser] == 0)
			{
				sprintf(climsg, "No players to play with!\n");
				write(newSocket, climsg, strlen(climsg) + 1);
				continue;
			}

			int pos1, pos2;
			sscanf(climsg, "%s %d %d", inp, &pos1, &pos2);

			if(pos1 < 0 || pos1 >= 3 || pos2 < 0 || pos2 >= 3 || cand[curBoard[curUser]] != curUser || board[curBoard[curUser]][3 * pos1 + pos2] != '.')
			{
				write(newSocket, "Invalid Move!\n", 14);

				if(cand[curBoard[curUser]] != curUser) write(newSocket, "Not your turn!\n", 15);
				if(board[curBoard[curUser]][3 * pos1 + pos2] != '.') write(newSocket, "Already has symbol\n", 19);

				continue;
			}

			if(curUser < playwith[curUser])
			{
				board[curBoard[curUser]][3 * pos1 + pos2] = 'x';
			}
			else
			{
				board[curBoard[curUser]][3 * pos1 + pos2] = 'o';
			}

			int cur = 0;
			for(int i = 0; i < 9; i++)
			{
				buff[cur++] = board[curBoard[curUser]][i];
				if(i % 3 == 2) buff[cur++] = '\n';
			}
			buff[cur] = 0;

			if(win(curBoard[curUser]) == 1) strcat(buff, "User Wins.\nEnd.\n");
			else if(win(curBoard[curUser]) == -1) strcat(buff, "Draw.\nEnd.\n");

			write(newSocket, buff, strlen(buff));
			write(playwith[curUser], buff, strlen(buff));

			if(win(curBoard[curUser]))
			{
				if(aite > 0)
				{
					playwith[aite] = 0;
				}
				playwith[curUser] = 0;
				aite = -1;
				strcpy(board[curBoard[curUser]], ".........");
			}

			cand[curBoard[curUser]] = playwith[curUser];
		}		
	}
	
	printf("exit thread %d...\n", newSocket);
	close(newSocket);
	pthread_exit(NULL);
}

int main()
{
	int svrSocket, newSocket[50];
	struct sockaddr_in svrAddr;
	struct sockaddr_storage svrStorage;
	socklen_t addr_size;

	svrSocket = socket(PF_INET, SOCK_STREAM, 0);

	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(PORT);
	svrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	memset(svrAddr.sin_zero, 0, sizeof(svrAddr.sin_zero));

	bind(svrSocket, (struct sockaddr *) &svrAddr, sizeof(svrAddr));
	listen(svrSocket, 10);

	board_init();

	memset(users, 0, sizeof(users));
	pthread_t tid[50];
	int tCount = 0;
printf("Initialized.\n");
	while(1)
	{
		addr_size = sizeof(svrStorage);
		newSocket[tCount] = accept(svrSocket, (struct sockaddr *) &svrStorage, &addr_size);

		if(pthread_create(&tid[tCount], NULL, thread, &newSocket[tCount]) != 0)
		{
			printf("Create Failed\n");
		}

		printf("Created thread tid = %d\n", (int)tid[tCount]);

		tCount++;
		tCount %= 50;
	}

	return 0;
}
