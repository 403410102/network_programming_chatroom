#include	"unpthread.h"

static void	*doit(void *);		/* each thread executes this function */
void serv_login(int );
void some_service(int);
void list_all_online_user(int);
void broadcast(int);
void send_someone_message(int);
void show_chat_history(int);
//void send_someone_file(int);
void p2preq(int,int);
void p2filepreq(int,int);
void p2pacc(int);
void leave(int);
int find_user(int);
int find_no(char*);
static int reqp2pfd;
static int accp2pfd;

char		option[MAXLINE]="\e[37m1.查看線上使用者 2.傳給給所有人訊息 3.傳送給某人訊息 4.查看聊天紀錄 !ps.發送p2p聊天要求 !pc.接受p2p聊天 !pr.拒絕p2p聊天 !fs.發送檔案傳輸要求 !fc.接受檔案 !fr.拒絕接受檔案 !q.離開\e[0m\n";

char chat_rec[MAXLINE][256];
int chat_rec_idx=0;

struct user{

	char name[100];
	char password[100];
	int online;
	int no;
}users[1000];

int user_cnt=0;
int p2pport=8081;

int main(int argc, char **argv)
{
	
	printf("%s",option);
	strcpy(users[0].name,"admin");
	strcpy(users[0].password,"admin");
	users[0].online=0;
	users[0].no=-1;
	user_cnt++;
	int	listenfd, *iptr;
	pthread_t		tid;
	socklen_t		addrlen, len;
	struct sockaddr	*cliaddr;

	if (argc == 2)
		listenfd = Tcp_listen(NULL, argv[1], &addrlen);
	else if (argc == 3)
		listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
	else
		err_quit("usage: tcpserv [ <host> ] <service or port>");

	cliaddr = Malloc(addrlen);

	for ( ; ; ) {
		len = addrlen;
		iptr = Malloc(sizeof(int));
		*iptr = Accept(listenfd, cliaddr, &len);
		Pthread_create(&tid, NULL, &doit, iptr);
	}
}

static void *doit(void *arg)
{
	int		connfd;

	connfd = *((int *) arg);
	free(arg);

	Pthread_detach(pthread_self());
	serv_login(connfd);
	some_service(connfd);
	//str_echo(connfd);		/* same function as before */
	Close(connfd);			/* done with connected socket */
//	printf("client closed\n");
	return(NULL);
}

void serv_login(int sockfd)
{
	ssize_t		n;
	char		buf[MAXLINE];
	char		welcome[MAXLINE]="Welcome,please login:\n";
	char		hello[MAXLINE]="hello, ";
	int i;

	Writen(sockfd,welcome,strlen(welcome));
	if(Readline(sockfd,buf,MAXLINE)<=0)
		return;
	//printf("new user: %s",buf);

	buf[strlen(buf)-1]='\0';

	strcat(hello,buf);
	strcat(hello,"\n");
	Writen(sockfd,hello,strlen(hello));

	for(i=0;i<user_cnt;i++)
		if(strcmp(users[i].name,buf)==0)
		{

			users[i].no=sockfd;
			break;
		}
	if(i==user_cnt)
	{
		
		strcpy(users[user_cnt].name,buf);
		users[user_cnt].online=1;
		users[user_cnt].no=sockfd;
		user_cnt++;

		sprintf(buf,"非已註冊帳號，請設定密碼:\n");
		Writen(sockfd,buf,strlen(buf));
		
		if(Readline(sockfd, buf, MAXLINE)<=0)
			return;
		buf[strlen(buf)-1]='\0';		
		sprintf(users[i].password,"%s",buf);
		sprintf(buf,"!loginok\n");
		Writen(sockfd,buf,strlen(buf));
		return;
	}
	sprintf(buf,"輸入密碼:\n");
	Writen(sockfd,buf,strlen(buf));
	if(Readline(sockfd, buf, MAXLINE)<=0)
			return;
	do
	{
		buf[strlen(buf)-1]='\0';
		if(strcmp(buf,users[i].password)==0)
		{
			sprintf(buf,"!loginok\n");
			Writen(sockfd,buf,strlen(buf));
			users[i].online=1;
			return;
		}
		sprintf(buf,"!loginnotok\n");
		Writen(sockfd,buf,strlen(buf));
		
	}while(Readline(sockfd, buf, MAXLINE)>0);
	users[i].online=1;

}

void some_service(int sockfd)
{
	ssize_t		n;
	char		buf[MAXLINE];
	char		tmp[MAXLINE];
	
	char		dash[MAXLINE]="~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
	Writen(sockfd,option,strlen(option));
	//char		serv_fini[MAXLINE]="server finished\n";
	for(;;)
	{
		Writen(sockfd,dash,strlen(dash));
	//	Writen(sockfd,serv_fini,strlen(serv_fini));
		if(Readline(sockfd,buf,MAXLINE)<=0)
			return;
		//printf("%s",buf);

		if(strcmp(buf,"1\n")==0)
			list_all_online_user(sockfd);
		else if(strcmp(buf,"2\n")==0)
			broadcast(sockfd);
		else if(strcmp(buf,"3\n")==0)
			send_someone_message(sockfd);
		else if(strcmp(buf,"4\n")==0)
			show_chat_history(sockfd);
		else if(strcmp(buf,"!ps\n")==0)
		{
			reqp2pfd=sockfd;
			//printf("reqp2pfd= %d\n",sockfd);
			//printf("server recive !ps\n");
			char tmp[MAXLINE]="Input ID:\n";
			char targetuser[MAXLINE];
			int tarno;
			Writen(sockfd,tmp,strlen(tmp));
			if(Readline(sockfd, targetuser, MAXLINE)<=0)
				return;
			targetuser[strlen(targetuser)-1]='\0';
			tarno=find_no(targetuser);
			if(tarno==-1)
			{
				sprintf(tmp,"此人不存在\n");
				Writen(sockfd,tmp,strlen(tmp));
				continue;
			}
			//printf("susseful\n");
			p2preq(sockfd,tarno);
		}
		else if(strcmp(buf,"!pc\n")==0)
		{
			sprintf(tmp,"!ok\n");

			Writen(reqp2pfd, tmp, strlen(tmp));
//			p2pacc(sockfd);
		}
		else if(strcmp(buf,"!pr\n")==0)
		{
			sprintf(buf,"!notok\n");
			Writen(reqp2pfd, buf, strlen(buf));
//			p2pacc(sockfd);
		}
		else if(strcmp(buf,"!fs\n")==0)
		{
			reqp2pfd=sockfd;
			//printf("reqp2pfd= %d\n",sockfd);
			//printf("server recive !ps\n");
			char tmp[MAXLINE]="Input ID:\n";
			char targetuser[MAXLINE];
			int tarno;
			Writen(sockfd,tmp,strlen(tmp));
			if(Readline(sockfd, targetuser, MAXLINE)<=0)
				return;
			targetuser[strlen(targetuser)-1]='\0';
			tarno=find_no(targetuser);
			if(tarno==-1)
			{
				sprintf(tmp,"此人不存在\n");
				Writen(sockfd,tmp,strlen(tmp));
				continue;
			}
//			printf("susseful\n");
			p2pfilereq(sockfd,tarno);
		}
		else if(strcmp(buf,"!fc\n")==0)
		{
			sprintf(tmp,"!ok\n");

			Writen(reqp2pfd, tmp, strlen(tmp));
//			p2pacc(sockfd);
		}
		else if(strcmp(buf,"!fr\n")==0)
		{
			sprintf(buf,"!notok\n");
			Writen(reqp2pfd, buf, strlen(buf));
//			p2pacc(sockfd);
		}
		else if(strcmp(buf,"!fc\n")==0)
		{
			sprintf(tmp,"!ok\n");
		//	printf("204\n");
			Writen(reqp2pfd, tmp, strlen(tmp));
//			p2pacc(sockfd);
		}
		else if(strcmp(buf,"!q\n")==0)
		{
			leave(sockfd);
			return;
		}
	}
}

void list_all_online_user(int sockfd)
{

	char		buf[MAXLINE]="";
	char		tmp[MAXLINE];
	int i;
	//printf("user_cnt= %d\n",user_cnt);
	if(users[0].online==1)
		sprintf(buf, "\e[1m\e[36m\t%s\e[0m", users[i].name);
	else if(users[0].online==0)
		sprintf(buf, "\e[36m\t%s\e[0m", users[i].name);

	for(i=1;i<user_cnt;i++)
	{
		if(users[i].online==1)
			sprintf(buf, "%s\e[1m\e[36m\t%s\e[0m", buf, users[i].name);

		else if(users[i].online==0)
			sprintf(buf, "%s\e[36m\t%s\e[0m", buf, users[i].name);
	}

	sprintf(buf,"%s\n",buf);

	Writen(sockfd,buf,strlen(buf));
}
void broadcast(int sockfd)
{

	int i;
	char		buf[MAXLINE];
	char		sendline[MAXLINE];
	char		thisuser[MAXLINE];

	for(i=0;i<user_cnt;i++)
	{
		if(users[i].no==sockfd)
		{
			strcpy(thisuser,users[i].name);
			break;
		}
	}

	while(Readline(sockfd, buf, MAXLINE) > 0)
	{
		if(strcmp(buf,"!q\n")==0)
			return;
		sprintf(sendline,"\e[1m\e[36m%s\e[0m  :%s",thisuser,buf);
		sprintf(chat_rec[chat_rec_idx],"%s",sendline);
		chat_rec_idx=chat_rec_idx+1;

		//printf("sendline context:%s\n",sendline);
		for(i=0;i<user_cnt;i++)
			if(users[i].online==1&&users[i].no!=-1&&users[i].no!=sockfd)
			{
				//printf("fd %d\n",users[i].no);
				Writen(users[i].no,sendline,strlen(sendline));
			}
	}

}

int find_no(char *targetuser)
{
	int i;
	for(i=0;i<user_cnt;i++)
		if(strcmp(targetuser,users[i].name)==0&&users[i].online==1)
			return users[i].no;		

	return -1;
}

int find_user(int sockfd)
{
	int i;

	for(i=0;i<user_cnt;i++)
	{
		//printf("%d %d\n",sockfd,users[i].no);
		if(users[i].no==sockfd&&users[i].online==1)
		{
			//printf("matched %s\n",users[i].name);
			return i;
		}		
	}
	return -1;
}

void send_someone_message(int sockfd)
{
	int i;
	int targetfd;
	char inputname[MAXLINE]="Input ID name:\n";
	char notexist[MAXLINE]="此人不存在\n";
	char		buf[MAXLINE];
	char		sendline[MAXLINE];
	char		thisuser[MAXLINE];
	char		targetuser[MAXLINE];
	for(i=0;i<user_cnt;i++)
		if(users[i].no==sockfd)
		{
			strcpy(thisuser,users[i].name);
			break;
		}

	Writen(sockfd,inputname,strlen(inputname));
	Readline(sockfd, targetuser, MAXLINE);
	targetuser[strlen(targetuser)-1]='\0';

	for(i=0;i<user_cnt;i++)
		if(strcmp(targetuser,users[i].name)==0&&users[i].online==1)
		{
			targetfd=users[i].no;
			
			break;
		}
	if(i==user_cnt)
	{
		Writen(sockfd,notexist,strlen(notexist));
		return;
	}
	
	while(Readline(sockfd, buf, MAXLINE) > 0)
	{
		if(strcmp(buf,"!q\n")==0)
			return;
		sprintf(sendline,"\e[1m\e[36m%s\e[0m  :%s",thisuser,buf);
		
		Writen(targetfd,sendline,strlen(sendline));

		sprintf(chat_rec[chat_rec_idx],"%s",sendline);
		chat_rec_idx=chat_rec_idx+1;
	}
}

void show_chat_history(int sockfd)
{
	char		sendline[MAXLINE];
	
	for(int i=0;i<chat_rec_idx;i++)
	{
		sprintf(sendline,"%s",chat_rec[i]);
		Writen(sockfd,sendline,strlen(sendline));
	}
	

}

/*
void send_someone_file(int sockfd)
{
	int i;
	int targetfd;


	char		buf[MAXLINE];
	char		sendline[MAXLINE];
	char		thisuser[MAXLINE];
	char		targetuser[MAXLINE];
	for(i=0;i<user_cnt;i++)
		if(users[i].no==sockfd)
		{
			strcpy(thisuser,users[i].name);
			break;
		}

	char 		inputname[MAXLINE]="Input ID name:\n";
	Writen(sockfd,inputname,strlen(inputname));
	Readline(sockfd, targetuser, MAXLINE);
	targetuser[strlen(targetuser)-1]='\0';

	for(i=0;i<user_cnt;i++)
		if(strcmp(targetuser,users[i].name)==0&&users[i].online==1)
		{
			targetfd=users[i].no;
			
			break;
		}
	char 		notexist[MAXLINE]="此人不存在\n";
	if(i==user_cnt)
	{
		Writen(sockfd,notexist,strlen(notexist));
		return;
	}

	char 		filepath[MAXLINE]="Input file path:\n";
	Writen(sockfd,filepath,strlen(filepath));

	char 		agree[MAXLINE];
	sprintf(agree,"是否接受來自%s的檔案:，按5接受\n",thisuser);
	Writen(targetfd,agree,strlen(agree));
	Readline(targetfd, buf, MAXLINE);
	printf(buf);

	if(buf[0]=='2')
	{
		char 		reject[MAXLINE];
		sprintf(reject,"%s拒絕接收檔案\n",targetuser);
		Writen(sockfd,reject,strlen(reject));
		return;
	}

	sprintf(agree,"接收方準備好傳送檔案\n");
	Writen(sockfd,agree,strlen(agree));

	sprintf(agree,"接收檔案中\n");
	Writen(targetfd,agree,strlen(agree));

	while(Readline(sockfd, buf, MAXLINE) > 0)
	{
	
		Writen(targetfd,buf,strlen(buf));
		
	}

}
*/

void p2preq(int sockfd,int targfd)
{
	char tmp[MAXLINE];
	int user_idx;

	if((user_idx=find_user(sockfd))==-1)
		return;
	//printf("%s\n",users[user_idx].name);

	sprintf(tmp,"\e[1m\e[36m%s\e[25m\e[93m發送p2p請求...\e[0m\n",users[user_idx].name);
	Writen(targfd,tmp,strlen(tmp));
	return;
//	if((thisuser=find_user(sockfd))==NULL)
//		return;

}

void p2pfilereq(int sockfd,int targfd)
{
	char tmp[MAXLINE];
	int user_idx;

	if((user_idx=find_user(sockfd))==-1)
		return;
	//printf("%s\n",users[user_idx].name);

	sprintf(tmp,"\e[1m\e[36m%s\e[25m\e[93m發送檔案傳輸請求...\e[0m\n",users[user_idx].name);
	Writen(targfd,tmp,strlen(tmp));
	return;
//	if((thisuser=find_user(sockfd))==NULL)
//		return;

}


void p2pacc(int sockfd)
{
	char buf[MAXLINE]="p2p-cli\n";
	Writen(sockfd,buf,strlen(buf));
}


void leave(int sockfd)
{
	int i;
	//char	close_msg[MAXLINE]="disconnected\n";

	for(i=0;i<user_cnt;i++)
	{
		if(users[i].no==sockfd)
		{
			users[i].online=0;
			break;
		}
	}
//	Writen(sockfd,close_msg,strlen(close_msg));
}





