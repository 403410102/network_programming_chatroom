#include	"unpthread.h"

void cli_work(FILE *,int);
void p2p_cli_work(FILE *,int);
void p2p_serv_work(FILE *,int);
void *send_something_to(void *arg);
void *p2p_cli_send_something_to(void *arg);
void *p2p_serv_send_something_to(void *arg);
void *p2p_cli_send_file_to(void *arg);
void *p2p_serv_send_file_to(void *arg);
static int	sockfd;
static int	p2psockfd;
static FILE	*fp;
static int	done;
int p2pflag=0;
int loginflag=0;
int p2pfinished=0;
char p2pport[10]="8085";
char p2pfport[10]="8086";
char thisuser[MAXLINE];



int main(int argc, char **argv)
{
	int		sockfd;

	if (argc != 3)
		err_quit("usage: tcpcli <hostname> <service>");

	sockfd = Tcp_connect(argv[1], argv[2]);
//	login(stdin,sockfd);
	cli_work(stdin, sockfd);		/* do it all */

	exit(0);
}

void cli_work(FILE *fp_arg, int sockfd_arg)
{
	char	recvline[MAXLINE];
	char	sendline[MAXLINE];
	pthread_t	tid;

	sockfd = sockfd_arg;	/* copy arguments to externals */
	fp = fp_arg;
	int n;
	

	Pthread_create(&tid, NULL, send_something_to, NULL);
	while (Readline(sockfd, recvline, MAXLINE) > 0)
	{
		//Fputs(recvline, stdout);
		if(strcmp(recvline,"!loginok\n")==0)
		{

			loginflag=1;
			//printf("p2pflag = %d\n",p2pflag);
			continue;
		}
		if(strcmp(recvline,"!loginnotok\n")==0)
		{

			loginflag=2;
			//printf("p2pflag = %d\n",p2pflag);
			continue;
		}
		if(strcmp(recvline,"!ok\n")==0)
		{

			p2pflag=1;
			//printf("p2pflag = %d\n",p2pflag);
			continue;
		}

		if(strcmp(recvline,"!notok\n")==0)
		{

			p2pflag=2;
			//printf("p2pflag = %d\n",p2pflag);
			continue;
		}

		Fputs(recvline, stdout);
	}
}

void *send_something_to(void *arg)
{
	char	sendline[MAXLINE];
	char	filepath[MAXLINE];
	char	buf[MAXLINE];
	int n;
	//input id
	if(Fgets(sendline, MAXLINE, fp) == NULL)
		return;
	Writen(sockfd, sendline, strlen(sendline));
	sendline[strlen(sendline)-1]='\0';
	sprintf(thisuser,"%s",sendline);
	//input passward:
	if(Fgets(sendline, MAXLINE, fp) == NULL)
		return;
	Writen(sockfd, sendline, strlen(sendline));
	while(loginflag==0)sleep(1);
	while(loginflag==2)
	{
		printf("\e[31m密碼錯誤，請重新輸入:\e[0m\n");
		if(Fgets(sendline, MAXLINE, fp) == NULL)
			return;
		Writen(sockfd, sendline, strlen(sendline));
	sleep(1);
	}
	printf("\e[32m進入聊天室\e[0m\n");
	loginflag=0;
//	printf("username:%s\n",thisuser);
	while (Fgets(sendline, MAXLINE, fp) != NULL)
	{
		p2pfinished=0;
		//printf("send_something_to\n");
		Writen(sockfd, sendline, strlen(sendline));
		if(strcmp(sendline,"!ps\n")==0)
		{
			build_serv();
			//printf("out of build serv\n");
			p2pflag=0;

			continue;
		}
		else if(strcmp(sendline,"!pc\n")==0)
		{
			build_cli();
			//printf("out of build cli\n");
			
			continue;
		}
		else if(strcmp(sendline,"!fs\n")==0)
		{
			build_file_serv();
			p2pflag=0;

			continue;
		}
		else if(strcmp(sendline,"!fc\n")==0)
		{
			build_file_cli();
			continue;
		}
		
		
	}

	Shutdown(sockfd, SHUT_WR);	/* EOF on stdin, send FIN */

	done = 1;
	return(NULL);
	/* return (i.e., thread terminates) when end-of-file on stdin */
}
//---------------------------------------------------------------------------
void build_serv()
{
	int	listenfd, *iptr,connfd,c;
//	pthread_t		tid;
	socklen_t		addrlen, len;
	struct sockaddr	*cliaddr;
	char sendline[MAXLINE];
	char recvline[MAXLINE];

	if(Fgets(sendline, MAXLINE, fp) == NULL)
		return;
	Writen(sockfd, sendline, strlen(sendline));
	while(p2pflag==0) sleep(.5);
	if(p2pflag==2)	//reject
	{
		printf("\e[31m要求被拒!!\e[0m\n");
		return;
	}
	if(p2pflag!=1)
		return;

	if(p2pflag==1)
	{
		printf("\e[32mserver building...\e[0m\n");
		
	}

	listenfd = Tcp_listen(NULL, p2pport, &addrlen);
	
	//-----------

	cliaddr = Malloc(addrlen);


	len = addrlen;
	iptr = Malloc(sizeof(int));
	//printf("before accept listenfd = %d\n",listenfd);
	connfd = Accept(listenfd, cliaddr, &len);
	p2p_serv_work(stdin, connfd);
	printf("\e[32m離開p2p聊天\e[0m\n");
	Close(listenfd);
	return;

	//-----------

}

void p2p_serv_work(FILE *fp_arg, int sockfd_arg)
{
	char	recvline[MAXLINE];
	char	sendline[MAXLINE];
	pthread_t	tid;

	p2psockfd = sockfd_arg;	/* copy arguments to externals */
	fp = fp_arg;
	int n;
	

	Pthread_create(&tid, NULL, p2p_serv_send_something_to, NULL);
	while (Readline(p2psockfd, recvline, MAXLINE) > 0)
	{
		Fputs(recvline, stdout);
		if(strcmp(recvline,"!q\n")==0)
		{
			p2pfinished=1;
			Close(p2psockfd);
			return;
		}
	}
}

void *p2p_serv_send_something_to(void *arg)
{
	char	sendline[MAXLINE];
	char	tmp[MAXLINE];
	while (Fgets(sendline, MAXLINE, fp) != NULL)
	{
		if(p2pfinished!=0)
			pthread_exit(NULL);

		if(strcmp(sendline,"!q\n")==0)
		{
			Writen(p2psockfd, sendline, strlen(sendline));
			//sleep(5);
			Shutdown(p2psockfd, SHUT_WR);
			p2pfinished=1;
			pthread_exit(NULL);
		}
		sprintf(tmp,"\e[1m\e[36m%s\e[0m  :%s",thisuser,sendline);
		Writen(p2psockfd, tmp, strlen(tmp));
	}
	

		/* EOF on stdin, send FIN */

	done = 1;
	return(NULL);
	/* return (i.e., thread terminates) when end-of-file on stdin */
}

void build_cli()
{

	int p2pfd;
	sleep(1);
	p2pfd = Tcp_connect("127.0.0.1", p2pport);
	p2p_cli_work(stdin, p2pfd);		/* do it all */
	printf("\e[32m離開p2p聊天\e[0m\n");

}

void p2p_cli_work(FILE *fp_arg, int sockfd_arg)
{
	char	recvline[MAXLINE];
	char	sendline[MAXLINE];
	pthread_t	tid;

	p2psockfd = sockfd_arg;	/* copy arguments to externals */
	fp = fp_arg;
	int n;
	

	Pthread_create(&tid, NULL, p2p_cli_send_something_to, NULL);
	while (Readline(p2psockfd, recvline, MAXLINE) > 0)
	{
		
		if(strcmp(recvline,"!q\n")==0)
		{
			p2pfinished=1;
			Close(p2psockfd);
			return;
		}
		Fputs(recvline, stdout);
	}
}

void *p2p_cli_send_something_to(void *arg)
{
	char	sendline[MAXLINE];
	char	tmp[MAXLINE];
	while (Fgets(sendline, MAXLINE, fp) != NULL)
	{
		if(p2pfinished!=0)
			pthread_exit(NULL);

		if(strcmp(sendline,"!q\n")==0)
		{
			Writen(p2psockfd, sendline, strlen(sendline));
			//sleep(5);
			Shutdown(p2psockfd, SHUT_WR);
			p2pfinished=1;
			pthread_exit(NULL);
		}
		sprintf(tmp,"\e[1m\e[36m%s\e[0m  :%s",thisuser,sendline);
		Writen(p2psockfd, tmp, strlen(tmp));
	}
	

		/* EOF on stdin, send FIN */

	done = 1;
	return(NULL);
	/* return (i.e., thread terminates) when end-of-file on stdin */
}


//---------------------------------------------------------------------------
void build_file_serv()
{
	int	listenfd, *iptr,connfd,c;
//	pthread_t		tid;
	socklen_t		addrlen, len;
	struct sockaddr	*cliaddr;
	char sendline[MAXLINE];
	char recvline[MAXLINE];

	if(Fgets(sendline, MAXLINE, fp) == NULL)
		return;
	Writen(sockfd, sendline, strlen(sendline));
	while(p2pflag==0) sleep(.5);
	if(p2pflag==2)	//reject
	{
		printf("\e[31m要求被拒!!\e[0m\n");
		return;
	}
	if(p2pflag!=1)
		return;

	if(p2pflag==1)
	{
		printf("\e[32mmserver building...\e[0m\n");
		
	}

	listenfd = Tcp_listen(NULL, p2pfport, &addrlen);
	
	//-----------

	cliaddr = Malloc(addrlen);

	len = addrlen;
	iptr = Malloc(sizeof(int));
	//printf("before accept listenfd = %d\n",listenfd);
	connfd = Accept(listenfd, cliaddr, &len);
	p2p_serv_file_work(stdin, connfd);
	printf("檔案傳輸完成。\n");
	Close(listenfd);
	return;

	//-----------

}

void p2p_serv_file_work(FILE *fp_arg, int sockfd_arg)
{
	char	buf[MAXLINE];
	char	file_name[MAXLINE];
	char	purename[MAXLINE];
	struct stat filestat;
	FILE *fp;
	pthread_t	tid;

	p2psockfd = sockfd_arg;	/* copy arguments to externals */
	fp = fp_arg;
	int n;
	puts("請輸入檔案:");
	gets(file_name);

	while(lstat(file_name, &filestat) < 0)
	{
		perror("\e[31merror\e[0m");
		puts("請輸入檔案:");
		gets(file_name);
	}
	for(int i=strlen(file_name)-1;i>=0;i--)
	{
		if(file_name[i]=='/')
		{
			int j;
			i++;
			for(j=0;i<strlen(file_name);i++,j++)
			{
				purename[j]=file_name[i];
			}
			purename[j]='\0';
			break;
		}
	}

	sprintf(buf,"%s\n",purename);
	Writen(p2psockfd, buf, strlen(buf));
	if(Readline(p2psockfd, buf, MAXLINE) > 0)
	{
		//printf("接收檔案：%s",buf);
	}
/*
	if ( lstat("./file.png", &filestat) < 0){
		perror("lstat");
		exit(1);
	}
*/
	//printf("The file size is %lun", filestat.st_size);
	
	fp = fopen(file_name, "rb");
	
	//Sending file
	while(!feof(fp)){
		n = fread(buf, sizeof(char), sizeof(buf), fp);
		//printf("fread %d bytes, \n", n);
		n = write(p2psockfd, buf, n);
		//printf("Sending %d bytesn\n",n);
	}

	Shutdown(p2psockfd,SHUT_WR);
	return;
}

void build_file_cli()
{

	int p2pfd;
	sleep(1);
	p2pfd = Tcp_connect("127.0.0.1", p2pfport);
	p2p_cli_file_work(stdin, p2pfd);		/* do it all */
	printf("檔案傳輸完成。\n");

}

void p2p_cli_file_work(FILE *fp_arg, int sockfd_arg)
{

	char	buf[MAXLINE];
	char	path[MAXLINE];
	char	tmp[MAXLINE];
	struct stat filestat;
	FILE *fp;
	pthread_t	tid;

	p2psockfd = sockfd_arg;	/* copy arguments to externals */
	fp = fp_arg;
	int n;
	printf("輸入接收檔案路徑:\n");
	gets(path);
	while(lstat(path, &filestat) < 0)
	{
		perror("\e[31merror\e[0m");
		puts("輸入接收檔案路徑:");
		gets(path);
	}
	sprintf(buf,"!ok\n");
	Writen(p2psockfd, buf, strlen(buf));
	if(Readline(p2psockfd, buf, MAXLINE) > 0)
	{
		printf("接收檔案：%s",buf);

	}
	buf[strlen(buf)-1]='\0';
	sprintf(tmp,"%s/%s",path,buf);
	sprintf(buf,"%s",tmp);
	if ( (fp = fopen(buf, "wb")) == NULL){
		perror("fopen");
		exit(1);
	}
	//Receive file from server
	while(1){
		n = read(p2psockfd, buf, sizeof(buf));
		//printf("read %d bytes, ", n);
		if(n == 0){
			break;
		}
		n = fwrite(buf, sizeof(char), n, fp);
		//printf("fwrite %d bytesn", n);
	}
	fclose(fp);
	close(p2psockfd);
	return;
	/*
	Pthread_create(&tid, NULL, p2p_cli_send_file_to, NULL);
	while (Readline(p2psockfd, recvline, MAXLINE) > 0)
	{
		Fputs(recvline, stdout);
		if(strcmp(recvline,"!exit\n")==0)
		{
			p2pfinished=1;
			Close(p2psockfd);
			return;
		}
	}
	*/
}

