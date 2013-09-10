#include <head.h>

#define CLIENT_LOGIN 10 
#define CLIENT_TALK 11
#define CLIENT_LOGOUT 12
#define SERVER_TALK 13
#define SERVER_LOGOUT 14

typedef struct
{
	int type;//消息的类型
	char name[15];//存放名字
	char buf[1024];//消息内容
}MSG;

void do_send(int sockfd,const char* pname,int pid,struct sockaddr_in* server_addr)
{
	MSG msg;
	int addr_len = sizeof(struct sockaddr);


	msg.type = CLIENT_LOGIN;
	strcpy(msg.name,pname);
	sendto(sockfd,&msg,sizeof(MSG),0,(struct sockaddr *)server_addr,sizeof(struct sockaddr));

	while(1)
	{
		int n;

		fgets(msg.buf,sizeof(msg.buf),stdin);
		msg.buf[strlen(msg.buf) - 1] = '\0';
		msg.type = CLIENT_TALK;
		if(sendto(sockfd,&msg,sizeof(MSG),0,(struct sockaddr *)server_addr,sizeof(struct sockaddr)) < 0)
		{
			perror("Fail to sendto");
			continue;
		}
		if(strncmp(msg.buf,"quit",4) == 0)
		{
			kill(pid,SIGUSR1);
			msg.type = CLIENT_LOGOUT;
			strcpy(msg.name,pname);
			sendto(sockfd,&msg,sizeof(MSG),0,(struct sockaddr *)server_addr,sizeof(struct sockaddr));
			return;
		}
	}

	return;
}

void do_recv(int sockfd)
{
	MSG msg;
	struct sockaddr peer_addr;
	int addr_len = sizeof(struct sockaddr);

	while(1)
	{
		recvfrom(sockfd,&msg,sizeof(MSG),0,&peer_addr,&addr_len);

		switch(msg.type)
		{
		case CLIENT_LOGIN:
			printf("*******CLIENT_LOGIN*******\n");
			printf("client %s is login in\n",msg.name);
			printf("*******CLIENT_LOGIN*******\n");
			break;

		case CLIENT_TALK:
			printf("*******CLIENT_TALK*******\n");
			printf("Name: %s\n",msg.name);
			printf("msg: %s\n",msg.buf);
			printf("*******CLIENT_TALK*******\n");
			break;

		case CLIENT_LOGOUT:
			printf("*******CLIENT_LOGOUT*******\n");
			printf("client %s is login out\n",msg.name);
			printf("*******CLIENT_LOGOUT*******\n");
			break;

		case SERVER_TALK:
			printf("*******SERVER_TALK*******\n");
			printf("Name: %s\n",msg.name);
			printf("msg: %s\n",msg.buf);
			printf("*******SERVER_TALK*******\n");
			break;
		case SERVER_LOGOUT:
			kill(getppid(),SIGUSR1);
			return;
		}
	}
}

int main(int argc, const char *argv[])
{
	int pid;
	int sockfd;
	struct sockaddr_in server_addr;

	if(argc < 4)
	{
		fprintf(stderr,"Usage %s server_ip server_port client_name\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	//创建数据包socket
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd < 0)
	{
		perror("Fail to socket");
		exit(EXIT_FAILURE);
	}

	bzero(&server_addr,sizeof(struct sockaddr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	if((pid = fork()) < 0)
	{
		perror("Fail to fork");
		exit(EXIT_FAILURE);
	}

	if(pid == 0)
	{
		do_recv(sockfd);
	}

	if(pid > 0)
	{
		do_send(sockfd,argv[3],pid,&server_addr);
	}

	return 0;
}
