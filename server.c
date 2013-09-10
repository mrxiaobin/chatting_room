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

typedef struct sockaddr DataType;
typedef struct node
{
	DataType data;
	char name[15];
	struct node *next;
}LinkList;

LinkList *creat_empty_linklist()
{
	LinkList *head = (LinkList *)malloc(sizeof(LinkList));
	
	head->next = NULL;

	return head;
}

//头插法
int insert_linklist(LinkList *head,char *name,DataType *data)
{
	LinkList *temp = NULL;
	temp = (LinkList *)malloc(sizeof(LinkList));

	strcpy(temp->name,name);
	temp->data = *data;
	temp->next = head->next;
	head->next = temp;

	return 0;
}

//删除指定节点
int delete_assign_linklist(LinkList *head,char *name)
{
	LinkList *p = head;
	LinkList *temp = NULL;
	while(p->next){
		if(strcmp(p->next->name,name) == 0)
		{
			break;
		}
		p = p->next;
		temp = temp->next;
	}
	temp = p->next;
	p->next = p->next->next;
	free(temp);

	return 0;
}

void broadcast_message(int socket_fd, LinkList *head, MSG *msg)  
{  
	LinkList *p = head->next;  

	while(p != NULL)  
	{  
		if (msg->type == CLIENT_LOGIN)  
		{  
			if (strcmp(p->name, msg->name) == 0)  
			{  
				p = p->next;  
				continue;  
			}  
		}  

		sendto(socket_fd,msg,sizeof(MSG), 0, &(p->data), sizeof(struct sockaddr));  
		p = p->next;  
	}  

	return ;  
}

void do_client(int sockfd,LinkList *head)
{
	MSG msg;
	struct sockaddr peer_addr;
	int addr_len = sizeof(struct sockaddr);

	//子进程: 接受消息，让后转发
	while(1)
	{
		recvfrom(sockfd,&msg,sizeof(MSG),0,&peer_addr,&addr_len);

		switch(msg.type)
		{
		case CLIENT_LOGIN:
			//第一次客户端登陆，子进程应该将客户端的地址保存在链表  
			insert_linklist(head,msg.name,&peer_addr);
			printf("*******CLIENT_LOGIN*******\n");
			printf("client %s is login in\n",msg.name);
			printf("*******CLIENT_LOGIN*******\n");
			broadcast_message(sockfd,head,&msg);
			break;

		case CLIENT_TALK:
			printf("*******CLIENT_TALK*******\n");
			printf("Name: %s\n",msg.name);
			printf("msg: %s\n",msg.buf);
			printf("*******CLIENT_TALK*******\n");
			broadcast_message(sockfd,head,&msg);
			break;

		case CLIENT_LOGOUT:
			printf("*******CLIENT_LOGOUT*******\n");
			printf("client %s is login out\n",msg.name);
			printf("*******CLIENT_LOGOUT*******\n");
			delete_assign_linklist(head,msg.name);
			broadcast_message(sockfd,head,&msg);
			break;

		case SERVER_TALK:
			printf("*******SERVER_TALK*******\n");
			printf("Name: %s\n",msg.name);
			printf("msg: %s\n",msg.buf);
			printf("*******SERVER_TALK*******\n");
			broadcast_message(sockfd,head,&msg);
			break;
		case SERVER_LOGOUT:
			kill(getppid(),SIGUSR1);
			broadcast_message(sockfd,head,&msg);
			return;

		}
	}
}

void do_send(int sockfd,struct sockaddr_in* server_addr)
{
	MSG msg;
	int addr_len = sizeof(struct sockaddr);

//	msg.type = SERVER_TALK;
//	strcpy(msg.name,"server");
//	sendto(sockfd,&msg,sizeof(msg),0,(struct sockaddr *)server_addr,sizeof(struct sockaddr));

	while(1)
	{
		int n;

		msg.type = SERVER_TALK;
		strcpy(msg.name,"server");
		fgets(msg.buf,sizeof(msg.buf),stdin);
		if(strncmp(msg.buf,"quit",4) == 0)
		{
			msg.type = SERVER_LOGOUT;
		}

		msg.buf[strlen(msg.buf) - 1] = '\0';
		if(sendto(sockfd,&msg,sizeof(msg),0,(struct sockaddr *)server_addr,sizeof(struct sockaddr)) < 0)
		{
			perror("Fail to sendto");
			continue;
		}
	}

	return;
}

int main(int argc, const char *argv[])
{
	int pid;
	int sockfd;
	LinkList *L = NULL;
	struct sockaddr_in server_addr;

	if(argc < 3)
	{
		fprintf(stderr,"Usage %s server_ip server_port\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	//创建数据包socket
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd < 0)
	{
		perror("Fail to socket");
		exit(EXIT_FAILURE);
	}

	//填充
	bzero(&server_addr,sizeof(struct sockaddr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	L = creat_empty_linklist();

	//绑定服务器ip和port
	if(bind(sockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) < 0)
	{
		perror("Fail to bind");
		exit(EXIT_FAILURE);
	}

	if((pid = fork()) < 0)
	{
		perror("Fail to fork");
		exit(EXIT_FAILURE);
	}

	if(pid == 0)
	{
		do_client(sockfd,L);
	}

	if(pid > 0)
	{
		do_send(sockfd,&server_addr);
	}

	return 0;
}
